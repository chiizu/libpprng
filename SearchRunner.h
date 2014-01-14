/*
  Copyright (C) 2011 chiizu
  chiizu.pprng@gmail.com
  
  This file is part of libpprng.
  
  libpprng is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  libpprng is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with libpprng.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SEARCH_RUNNER_H
#define SEARCH_RUNNER_H

#include <deque>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <functional>

namespace pprng
{

class SearchRunner
{
public:
  struct SearchDetails
  {
    uint32_t  threadNumber;
    uint64_t  totalSeeds;
    
    SearchDetails(uint32_t threadNumber_, uint64_t totalSeeds_)
      : threadNumber(threadNumber_), totalSeeds(totalSeeds_)
    {}
  };
  
  
  struct SearchProgress
  {
    const SearchDetails  &details;
    
    uint64_t  seedsSearched;
    
    SearchProgress(const SearchDetails &details_)
      : details(details_), seedsSearched(0)
    {}
  };
  
  
  struct StatusHandler
  {
    virtual ~StatusHandler() {}
    
    virtual void OnSearchDetails(const SearchDetails &) = 0;
    virtual void OnSearchProgress(const SearchProgress &) = 0;
    virtual bool OnPauseCheck() = 0;
  };
  
  
  static uint32_t NumThreads() { return boost::thread::hardware_concurrency(); }
  
  
  // start a single-threaded search
  template <class SeedGenerator, class SeedSearcher, class ResultChecker,
            class ResultCallback>
  void Search(SeedGenerator &seedGenerator,
              SeedSearcher &seedSearcher,
              ResultChecker &resultChecker,
              ResultCallback &resultHandler,
              StatusHandler &statusHandler)
  {
    SearchDetails  details(0, seedGenerator.NumberOfSeeds());
    
    statusHandler.OnSearchDetails(details);
    
    Search(seedGenerator, seedSearcher, resultChecker, resultHandler,
           statusHandler, details, 0);
  }
  
  
  // start a new, threaded search
  template <class SeedGenerator, class SeedSearcher, class ResultChecker,
            class ResultCallback>
  void SearchThreaded(SeedGenerator &seedGenerator,
                      SeedSearcher &seedSearcher,
                      ResultChecker &resultChecker,
                      ResultCallback &resultHandler,
                      StatusHandler &statusHandler)
  {
    std::list<SeedGenerator>  generators = seedGenerator.Split(NumThreads());
    
    std::vector<uint64_t>     startingSeeds(generators.size(), 0);
    
    SearchThreaded(generators, seedSearcher, resultChecker, resultHandler,
                   statusHandler, startingSeeds);
  }
  
  
  // continue a previous threaded search
  template <class SeedGenerator, class SeedSearcher, class ResultChecker,
            class ResultCallback>
  void ContinueSearchThreaded(SeedGenerator &seedGenerator,
                              SeedSearcher &seedSearcher,
                              ResultChecker &resultChecker,
                              ResultCallback &resultHandler,
                              StatusHandler &statusHandler,
                              const std::vector<uint64_t> &startingSeeds)
  {
    std::list<SeedGenerator>  generators =
      seedGenerator.Split(startingSeeds.size());
    
    SearchThreaded(generators, seedSearcher, resultChecker, resultHandler,
                   statusHandler, startingSeeds);
  }
  
  
private:
  // main search loop
  template <class SeedGenerator, class SeedSearcher, class ResultChecker,
            class ResultCallback>
  void Search(SeedGenerator &seedGenerator,
              SeedSearcher &seedSearcher,
              ResultChecker &resultChecker,
              ResultCallback &resultHandler,
              StatusHandler &statusHandler,
              const SearchDetails &searchDetails,
              uint64_t startSeedNum)
  {
    typedef typename SeedGenerator::SeedType  SeedType;
    
    uint64_t  numSeeds = seedGenerator.NumberOfSeeds();
    
    double  seedPercent = double(seedGenerator.SeedsPerChunk()) / numSeeds;
    
    if (seedPercent > 0.002)
      seedPercent = 0.002;
    
    uint64_t  seedsPerCallback = (seedPercent * numSeeds) + 1;
    
    SearchProgress  searchProgress(searchDetails);
    
    if (startSeedNum > 0)
      seedGenerator.SkipSeeds(startSeedNum);
    
    uint64_t  i = startSeedNum, threshold = i;
    
    // check once at the beginning in case we're starting paused
    bool  continueSearch = statusHandler.OnPauseCheck();
    
    while ((i < searchDetails.totalSeeds) && continueSearch)
    {
      threshold += seedsPerCallback;
      
      if (threshold > searchDetails.totalSeeds)
        threshold = searchDetails.totalSeeds;
      
      for (/* empty */; i < threshold; ++i)
      {
        SeedType  seed = seedGenerator.Next();
        
        seedSearcher.Search(seed, resultChecker, resultHandler);
      }
      
      searchProgress.seedsSearched = threshold;
      statusHandler.OnSearchProgress(searchProgress);
      
      continueSearch = statusHandler.OnPauseCheck();
    }
  }
  
  
  typedef std::deque<SearchProgress>  ProgressQueue;
  
  template <class ResultType>
  struct CommonThreadData
  {
    typedef std::deque<ResultType>  ResultQueue;
    
    CommonThreadData(boost::condition_variable &condVar_, boost::mutex &mutex_,
                     ProgressQueue &progressQueue_, ResultQueue &resultQueue_,
                     bool &shouldContinue_, uint32_t &numActiveThreads_)
      : condVar(condVar_), mutex(mutex_), progressQueue(progressQueue_),
        resultQueue(resultQueue_), shouldContinue(shouldContinue_),
        numActiveThreads(numActiveThreads_)
    {}
    
    boost::condition_variable  &condVar;
    boost::mutex               &mutex;
    ProgressQueue              &progressQueue;
    ResultQueue                &resultQueue;
    bool                       &shouldContinue;
    uint32_t                   &numActiveThreads;
  };
  
  
  // spawn threads and monitor for progress and results
  template <class SeedGenerator, class SeedSearcher, class ResultChecker,
            class ResultCallback>
  void SearchThreaded(std::list<SeedGenerator> &generators,
                      SeedSearcher &seedSearcher,
                      ResultChecker &resultChecker,
                      ResultCallback &resultHandler,
                      StatusHandler &statusHandler,
                      const std::vector<uint64_t> &startingSeeds)
  {
    typedef typename SeedSearcher::ResultType  ResultType;
    typedef std::deque<ResultType>             ResultQueue;
    
    uint32_t  numThreads = generators.size();
    
    typename std::list<SeedGenerator>::iterator  sg = generators.begin();
    std::vector<uint64_t>::const_iterator        ss = startingSeeds.begin();
    
    typedef std::vector<SearchDetails>  DetailsVector;
    DetailsVector  detailsVector;
    
    for (uint32_t i = 0; i < numThreads; ++i, ++sg, ++ss)
    {
      detailsVector.push_back(SearchDetails(i, sg->NumberOfSeeds()));
      
      statusHandler.OnSearchDetails(*detailsVector.rbegin());
      
      SearchProgress  progress(*detailsVector.rbegin());
      progress.seedsSearched = *ss;
      
      statusHandler.OnSearchProgress(progress);
    }
    
    // check if we are starting paused
    if (!statusHandler.OnPauseCheck())
      return;  // all that for nothing!
    
    uint32_t  numActiveThreads = numThreads;
    
    boost::condition_variable  condVar;
    boost::mutex               mutex;
    ProgressQueue              progressQueue;
    ResultQueue                resultQueue;
    bool                       shouldContinue = true;
    
    CommonThreadData<ResultType>  commonData(condVar, mutex, progressQueue,
                                             resultQueue, shouldContinue,
                                             numActiveThreads);
    
    typedef std::list<boost::shared_ptr<boost::thread> >  ThreadList;
    ThreadList  threadList;
    
    typedef SearchFunctor<SeedGenerator, SeedSearcher, ResultChecker>
            SearchFunctor;
    
    sg = generators.begin();
    ss = startingSeeds.begin();
    
    DetailsVector::iterator  dt = detailsVector.begin();
    for (; dt != detailsVector.end(); ++dt, ++sg, ++ss)
    {
      SearchFunctor  searchFunctor(*this, commonData, *sg, seedSearcher,
                                   resultChecker, *dt, *ss);
      
      boost::shared_ptr<boost::thread>  t(new boost::thread(searchFunctor));
      
      threadList.push_back(t);
    }
    
    while (numActiveThreads > 0)
    {
      boost::unique_lock<boost::mutex>  lock(mutex);
      
      if (progressQueue.empty() && (numActiveThreads > 0))
        condVar.wait(lock);
      
      // look for new results
      while (!resultQueue.empty())
      {
        resultHandler(resultQueue.front());
        resultQueue.pop_front();
      }
      
      // update progress
      while (!progressQueue.empty())
      {
        statusHandler.OnSearchProgress(progressQueue.front());
        
        progressQueue.pop_front();
      }
      
      shouldContinue = shouldContinue && statusHandler.OnPauseCheck();
    }
    
    ThreadList::iterator  it;
    for (it = threadList.begin(); it != threadList.end(); ++it)
      (*it)->join();
  }
  
  // handle callbacks from each thread and queue them for handling
  //   on the main thread
  template <typename ResultType>
  struct ThreadResultHandler
  {
    typedef std::deque<ResultType>  ResultQueue;
    
    ThreadResultHandler(ResultQueue &resultQueue)
      : m_threadResultQueue(resultQueue)
    {}
    
    void operator()(const ResultType &result)
    {
      m_threadResultQueue.push_back(result);
    }
    
    ResultQueue  &m_threadResultQueue;
  };
  
  
  template <typename ResultType>
  struct ThreadStatusHandler  : public StatusHandler
  {
    typedef std::deque<ResultType>        ResultQueue;
    typedef CommonThreadData<ResultType>  CommonThreadData;
    
    ThreadStatusHandler(CommonThreadData &commonData,
                        ResultQueue &threadResultQueue)
      : m_commonData(commonData), m_threadResultQueue(threadResultQueue)
    {}
    
    void OnSearchDetails(const SearchDetails &)
    {}
    
    void OnSearchProgress(const SearchProgress &searchProgress)
    {
      {
        boost::unique_lock<boost::mutex>  lock(m_commonData.mutex);
        
        if (!m_threadResultQueue.empty())
        {
          m_commonData.resultQueue.insert(m_commonData.resultQueue.end(),
                                          m_threadResultQueue.begin(),
                                          m_threadResultQueue.end());
          
          m_threadResultQueue.clear();
        }
        
        m_commonData.progressQueue.push_back(searchProgress);
        
        m_commonData.condVar.notify_one();
      }
    }
    
    bool OnPauseCheck()
    {
      {
        boost::unique_lock<boost::mutex>  lock(m_commonData.mutex);
      }
      
      return m_commonData.shouldContinue;
    }
    
    CommonThreadData  &m_commonData;
    ResultQueue       &m_threadResultQueue;
  };
  
  
  // function object that each thread invokes
  template <class SeedGenerator, class SeedSearcher, class ResultChecker>
  struct SearchFunctor
  {
    typedef typename SeedSearcher::ResultType  ResultType;
    typedef std::deque<ResultType>             ResultQueue;
    typedef CommonThreadData<ResultType>       CommonThreadData;
    typedef ThreadResultHandler<ResultType>    ThreadResultHandler;
    typedef ThreadStatusHandler<ResultType>    ThreadStatusHandler;
    
    SearchFunctor(SearchRunner &searcher,
                  CommonThreadData &commonData,
                  SeedGenerator &seedGenerator,
                  SeedSearcher &seedSearcher,
                  ResultChecker &resultChecker,
                  SearchDetails &searchDetails,
                  uint64_t startingSeedNumber)
      : m_searcher(searcher), m_commonThreadData(commonData),
        m_seedGenerator(seedGenerator), m_seedSearcher(seedSearcher),
        m_resultChecker(resultChecker), m_searchDetails(searchDetails),
        m_startingSeedNumber(startingSeedNumber)
    {}
    
    void operator()()
    {
      ResultQueue          threadResultQueue;
      ThreadResultHandler  threadResultHandler(threadResultQueue);
      ThreadStatusHandler  threadStatusHandler(m_commonThreadData,
                                               threadResultQueue);
      
      m_searcher.Search(m_seedGenerator, m_seedSearcher, m_resultChecker,
                        threadResultHandler, threadStatusHandler,
                        m_searchDetails, m_startingSeedNumber);
      
      boost::unique_lock<boost::mutex>  lock(m_commonThreadData.mutex);
      
      --m_commonThreadData.numActiveThreads;
      
      m_commonThreadData.condVar.notify_one();
    }
    
    SearchRunner      &m_searcher;
    CommonThreadData  &m_commonThreadData;
    SeedGenerator     &m_seedGenerator;
    SeedSearcher      &m_seedSearcher;
    ResultChecker     &m_resultChecker;
    SearchDetails     &m_searchDetails;
    uint64_t          m_startingSeedNumber;
  };
};

}

#endif
