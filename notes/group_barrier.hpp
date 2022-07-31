#include <vector>
#include <string>
#include <algorithm>
#include <functional>
#include <cassert>
#include <polaris/utility/macros.hpp>
#include <polaris/utility/pretty_print.hpp>

class ActionGroup {
 public:
  ActionGroup(const std::vector<std::string>& sources,
              const std::vector<std::string>& sourceIndexMap)
      : mName(_GetGroupName(sources)),
        mGroupId(_CaculateGroupId(sources, sourceIndexMap)) {
    assert(!sources.empty());
  }

  const std::string& GetName() const { return mName; }

  // Group Id is the uint64 representation of the required sources as 64bits masks
  // If the group is (A, B) and {A: 0, B: 2}, then the group id is 0b0101 = 4 + 1 = 5
  std::uint64_t GetGroupId() const { return mGroupId; }

  // Returns true if all required sources have been marketed as active in the input
  // activeSources bits. For example:
  // Group Id is:         01001001
  // Active Sources are:  01111001   => return true
  // Active Sources are:  10111111   => return false
  bool CanTrigger(std::uint64_t activeSources) const {
    return mGroupId == (mGroupId & activeSources);
  }

 private:
  std::string   mName;
  std::uint64_t mGroupId;

  std::string _GetGroupName(const std::vector<std::string>& sources) {
    std::ostringstream oss;
    oss << sources;
    return oss.str();
  }
  std::uint64_t _CaculateGroupId(const std::vector<std::string>& sources,
                                 const std::vector<std::string>& sourceIndexMap) {
    std::uint64_t id = 0;
    for (const auto& source : sources) {
      auto it = std::find(sourceIndexMap.begin(), sourceIndexMap.end(), source);
      assert(it != sourceIndexMap.end());
      int idx = std::distance(sourceIndexMap.begin(), it);
      id |= 1ul << idx;
    }
    return id;
  }
};

class GroupBarrier {
 public:
  GroupBarrier() = default;
  GroupBarrier(const std::vector<std::vector<std::string> >& groups) { Init(groups); }
  void Init(const std::vector<std::vector<std::string> >& groups) {
    for (const auto& sources : groups) {
      for (const auto& source : sources) {
        mSources.emplace_back(source);
      }
    }

    // Sort sources and get unique sources firstly
    std::sort(mSources.begin(), mSources.end());
    mSources.erase(std::unique(mSources.begin(), mSources.end()), mSources.end());

    size_t numOfSources = mSources.size();
    ExpectTrue(numOfSources <= 64,
               std::runtime_error,
               "GroupBarrier numOfSources=[{}] exceed limit=[64] already!",
               numOfSources);
    mSourceDataCache.reserve(numOfSources);

    for (size_t i = 0; i < mSources.size(); ++i) {
      mSourceDataCache.emplace_back();
      constexpr size_t DEFAULT_CACHE_ITEM_SIZE = 512;
      mSourceDataCache.back().reserve(DEFAULT_CACHE_ITEM_SIZE);
    }

    for (const auto& sources : groups) {
      mGroups.emplace_back(sources, mSources);
    }
  }

  int GetSourceIndex(const std::string& source) const {
    auto it = std::find(mSources.begin(), mSources.end(), source);
    assert(it != mSources.end());
    return std::distance(mSources.begin(), it);
  }

  using GroupIter            = std::vector<const ActionGroup*>::const_iterator;
  using TriggeredGroupsRange = std::pair<GroupIter, GroupIter>;
  using Visitor              = std::function<void(const char* data, size_t size)>;
  std::pair<bool, TriggeredGroupsRange> ArriveSource(int            sourceIdx,
                                                     const Visitor& visitor,
                                                     const char*    data,
                                                     size_t         size,
                                                     bool           visitCurrentSource = true) {
    mSourceDataCache[sourceIdx].assign(data, data + size);

    auto [sources, groupRange] = _ArriveSource(sourceIdx);
    if (!sources) {
      // nothing to trigger
      return std::make_pair(false, groupRange);
    } else {
      int idx = 0;
      while (sources) {
        if (idx == sourceIdx && visitCurrentSource) {
          visitor(data, size);
        } else if (sources & 1ul) {
          visitor(mSourceDataCache[idx].data(), mSourceDataCache[idx].size());
        }
        sources >>= 1;
        ++idx;
      }
    }
    return std::make_pair(true, groupRange);
  }
  bool AllTriggered() const { return mAlreadyTriggeredGroups.size() == mGroups.size(); }

  void Reset() {
    mActivedSources = 0ul;
    mAlreadyTriggeredGroups.clear();
    for (auto& data : mSourceDataCache) {
      data.clear();
    }
  }

 private:
  friend class GroupBarrierTestSuite;
  std::vector<std::string> mSources;
  using Groups = std::vector<ActionGroup>;
  Groups                          mGroups;
  std::uint64_t                   mActivedSources = 0ul;
  std::vector<const ActionGroup*> mAlreadyTriggeredGroups;
  std::vector<std::string>        mSourceDataCache;

 private:
  // Once any source got active, then it will mark internal active sources
  // and also check if we can trigger any group/groups
  std::pair<std::uint64_t, TriggeredGroupsRange> _ArriveSource(int sourceIdx) {
    mActivedSources |= 1ul << sourceIdx;
    std::uint64_t requiredSources = 0ul;

    int triggeredGroups = 0;
    for (const auto& grp : mGroups) {
      auto it = std::find(mAlreadyTriggeredGroups.begin(), mAlreadyTriggeredGroups.end(), &grp);
      // Not yet triggered group
      if (it == mAlreadyTriggeredGroups.end()) {
        if (grp.CanTrigger(mActivedSources)) {
          requiredSources |= grp.GetGroupId();
          mAlreadyTriggeredGroups.emplace_back(&grp);
          ++triggeredGroups;
        }
      }
    }
    return std::make_pair(requiredSources,
                          std::make_pair(mAlreadyTriggeredGroups.end() - triggeredGroups,
                                         mAlreadyTriggeredGroups.end()));
  }
};

template <typename T, size_t Size>
class TimeSequenceBuffer {
 public:
  TimeSequenceBuffer() = default;

  template <typename... Args>
  void Init(Args&&... args) {
    for (auto& item : mEvents) {
      { item.second.Init(std::forward<Args>(args)...); }
    }
  }

  using InsertTime = std::uint64_t;
  using BufferType = std::array<std::pair<InsertTime, T>, Size>;

  // forward declration
  template <typename Item>
  class Iterator;
  using iterator       = Iterator<T>;
  using const_iterator = Iterator<const T>;

  // Get new buffer for events
  iterator GetBuffer(std::uint64_t insertTime) {
    if (mWriteIndex - mReadIndex == Size) {
      // Will overwrite buffer pending for purging
      iterator it{mReadIndex, this};
      it->Reset();
      ++mReadIndex;
    }

    iterator ret{mWriteIndex++, this};
    ret.SetTime(insertTime);

    return ret;
  }

  template <typename UnaryPredicate>
  iterator FindIf(UnaryPredicate p) {
    return std::find_if(begin(), end(), p);
  }

  // purge data to certain position
  void PurgeUntil(iterator itPos) {
    for (auto it = begin(); it != itPos; ++it) {
      it->Reset();
      ++mReadIndex;
    }
    // purge current one as well
    itPos->Reset();
    ++mReadIndex;
  }

  void PurgeUntil(std::uint64_t time) {
    // Items are ordered by insert time
    for (auto it = begin(); it.GetTime() <= time && it != end(); ++it) {
      it->Reset();
      ++mReadIndex;
    }
  }

 public:
  BufferType    mEvents;
  std::uint64_t mReadIndex  = 0;
  std::uint64_t mWriteIndex = 0;

  template <typename Item>
  class Iterator : public std::iterator<std::forward_iterator_tag, Item> {
   private:
    std::uint64_t       idx     = 0;
    TimeSequenceBuffer* pBuffer = nullptr;

   public:
    Iterator(std::uint64_t i = 0, TimeSequenceBuffer* p = nullptr)
        : idx(i),
          pBuffer(p) {}

    void          SetTime(std::uint64_t time) { pBuffer->mEvents[idx % Size].first = time; }
    std::uint64_t GetTime() const { return pBuffer->mEvents[idx % Size].first; }

    Item& operator*() { return pBuffer->mEvents[idx % Size].second; }
    Item* operator->() { return &(pBuffer->mEvents[idx % Size].second); }

    Iterator& operator++() {
      idx++;
      return *this;
    }

    Iterator operator++(int) {
      Iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    friend bool operator==(Iterator lhs, Iterator rhs) {
      return lhs.idx == rhs.idx && lhs.pBuffer == rhs.pBuffer;
    }

    friend bool operator!=(Iterator lhs, Iterator rhs) {
      return lhs.idx != rhs.idx || lhs.pBuffer != rhs.pBuffer;
    }

    // iterator -> const_iterator
    operator Iterator<const T>() const { return Iterator<const T>(idx, pBuffer); }
  };

  iterator       begin() { return iterator{mReadIndex, this}; }
  const_iterator begin() const { return const_iterator{mReadIndex, this}; }
  iterator       end() { return iterator{mWriteIndex, this}; }
  const_iterator end() const { return iterator{mWriteIndex, this}; }
  size_t         size() const { return mWriteIndex - mReadIndex; }
};