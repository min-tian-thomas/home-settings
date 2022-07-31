#include "tests/unit/utility/group_barrier.hpp"

#include <gtest/gtest.h>

class GroupBarrierTestSuite : public ::testing::Test {
 public:
  bool TestArriveSource1() {
    GroupBarrier barrier({{"A", "B"}, {"C", "B"}});
    auto         sourceA = barrier.GetSourceIndex("A");
    auto         sourceB = barrier.GetSourceIndex("B");
    auto         sourceC = barrier.GetSourceIndex("C");
    {
      // firstly A gets atived
      auto [sources, groupRange] = barrier._ArriveSource(sourceA);
      EXPECT_EQ(0ul, sources);
      EXPECT_TRUE(groupRange.first == groupRange.second);
    }
    {
      // secondly B gets atived
      auto [sources, groupRange] = barrier._ArriveSource(sourceB);
      // A and B can be triggered as a group
      EXPECT_EQ(0b0011, sources);
      EXPECT_EQ(1, std::distance(groupRange.first, groupRange.second));
      auto it = groupRange.first;
      EXPECT_EQ("[A, B]", (*it)->GetName());
      EXPECT_FALSE(barrier.AllTriggered());
    }
    {
      // thirdly C gets atived
      auto [sources, groupRange] = barrier._ArriveSource(sourceC);
      // B and C can be triggered as a group
      EXPECT_EQ(0b0110, sources);
      EXPECT_EQ(1, std::distance(groupRange.first, groupRange.second));
      auto it = groupRange.first;
      EXPECT_EQ("[C, B]", (*it)->GetName());
      EXPECT_TRUE(barrier.AllTriggered());
    }

    return true;
  }

  bool TestArriveSource2() {
    GroupBarrier barrier({{"A", "B"}, {"C", "B"}});
    auto         sourceA = barrier.GetSourceIndex("A");
    auto         sourceB = barrier.GetSourceIndex("B");
    auto         sourceC = barrier.GetSourceIndex("C");
    {
      // firstly A gets atived
      auto [sources, groupRange] = barrier._ArriveSource(sourceA);
      EXPECT_EQ(0ul, sources);
      EXPECT_TRUE(groupRange.first == groupRange.second);
    }
    {
      // secondly C gets atived
      auto [sources, groupRange] = barrier._ArriveSource(sourceC);
      EXPECT_EQ(0ul, sources);
      EXPECT_EQ(groupRange.first, groupRange.second);
      EXPECT_FALSE(barrier.AllTriggered());
    }
    {
      // thirdly B gets atived
      auto [sources, groupRange] = barrier._ArriveSource(sourceB);
      // A, B and C can be triggered all together with 2 groups
      EXPECT_EQ(0b0111, sources);
      EXPECT_EQ(2, std::distance(groupRange.first, groupRange.second));
      auto it = groupRange.first;
      EXPECT_EQ("[A, B]", (*it++)->GetName());
      EXPECT_EQ("[C, B]", (*it++)->GetName());
      EXPECT_EQ(it, groupRange.second);
      EXPECT_TRUE(barrier.AllTriggered());
    }
    return true;
  }

  bool TestReset() {
    GroupBarrier barrier({{"A", "B"}, {"C", "B"}, {"C"}});
    auto         sourceA = barrier.GetSourceIndex("A");
    auto         sourceB = barrier.GetSourceIndex("B");
    auto         sourceC = barrier.GetSourceIndex("C");
    {
      // firstly A gets atived
      auto [sources, groupRange] = barrier._ArriveSource(sourceA);
      EXPECT_EQ(0ul, sources);
      EXPECT_TRUE(groupRange.first == groupRange.second);
    }
    {
      // secondly C gets atived
      auto [sources, groupRange] = barrier._ArriveSource(sourceC);
      // C can be triggered alone
      EXPECT_EQ(0b0100, sources);
      EXPECT_EQ(1, std::distance(groupRange.first, groupRange.second));
      auto it = groupRange.first;
      EXPECT_EQ("[C]", (*it++)->GetName());
      EXPECT_EQ(it, groupRange.second);
      EXPECT_FALSE(barrier.AllTriggered());
    }
    {
      // thirdly B gets atived
      auto [sources, groupRange] = barrier._ArriveSource(sourceB);
      // A, B and C can be triggered all together with 2 groups
      EXPECT_EQ(0b0111, sources);
      EXPECT_EQ(2, std::distance(groupRange.first, groupRange.second));
      auto it = groupRange.first;
      EXPECT_EQ("[A, B]", (*it++)->GetName());
      EXPECT_EQ("[C, B]", (*it++)->GetName());
      EXPECT_EQ(it, groupRange.second);
      EXPECT_TRUE(barrier.AllTriggered());
    }

    // Check reset function here
    barrier.Reset();
    EXPECT_FALSE(barrier.AllTriggered());

    {
      // firstly B gets atived
      auto [sources, groupRange] = barrier._ArriveSource(sourceB);
      EXPECT_EQ(0ul, sources);
      EXPECT_TRUE(groupRange.first == groupRange.second);
    }
    {
      // firstly A gets atived
      auto [sources, groupRange] = barrier._ArriveSource(sourceA);
      EXPECT_EQ(0b0011, sources);
      EXPECT_EQ(1, std::distance(groupRange.first, groupRange.second));
      auto it = groupRange.first;
      EXPECT_EQ("[A, B]", (*it++)->GetName());
      EXPECT_EQ(it, groupRange.second);
      EXPECT_FALSE(barrier.AllTriggered());
    }
    {
      // firstly C gets atived
      auto [sources, groupRange] = barrier._ArriveSource(sourceC);
      EXPECT_EQ(0b0110, sources);
      EXPECT_EQ(2, std::distance(groupRange.first, groupRange.second));
      auto it = groupRange.first;
      EXPECT_EQ("[C, B]", (*it++)->GetName());
      EXPECT_EQ("[C]", (*it++)->GetName());
      EXPECT_EQ(it, groupRange.second);
      EXPECT_TRUE(barrier.AllTriggered());
    }
    return true;
  }
};

TEST(GroupBarrierTest, TestActionGroup) {
  ActionGroup grpAB({"A", "B"}, {"A", "B", "C"});
  EXPECT_EQ("[A, B]", grpAB.GetName());
  EXPECT_EQ(0b0011, grpAB.GetGroupId());
  EXPECT_TRUE(grpAB.CanTrigger(0b0011));
  EXPECT_TRUE(grpAB.CanTrigger(0b0111));
  EXPECT_TRUE(grpAB.CanTrigger(0b1011));
  EXPECT_TRUE(grpAB.CanTrigger(0b1111));
  EXPECT_FALSE(grpAB.CanTrigger(0b0010));
  EXPECT_FALSE(grpAB.CanTrigger(0b1010));
  EXPECT_FALSE(grpAB.CanTrigger(0b0101));
  EXPECT_FALSE(grpAB.CanTrigger(0b1101));

  ActionGroup grpBC({"C", "B"}, {"A", "B", "C"});
  EXPECT_EQ("[C, B]", grpBC.GetName());
  EXPECT_EQ(0b0110, grpBC.GetGroupId());
  EXPECT_TRUE(grpBC.CanTrigger(0b0111));
  EXPECT_TRUE(grpBC.CanTrigger(0b0110));
  EXPECT_TRUE(grpBC.CanTrigger(0b1111));
  EXPECT_TRUE(grpBC.CanTrigger(0b1110));
  EXPECT_FALSE(grpBC.CanTrigger(0b0010));
  EXPECT_FALSE(grpBC.CanTrigger(0b1010));
  EXPECT_FALSE(grpBC.CanTrigger(0b0101));
  EXPECT_FALSE(grpBC.CanTrigger(0b0001));
}

TEST(GroupBarrierTest, TestConstructor) {
  GroupBarrier barrier({{"A", "B"}, {"C", "B"}});
  EXPECT_EQ(0, barrier.GetSourceIndex("A"));
  EXPECT_EQ(1, barrier.GetSourceIndex("B"));
  EXPECT_EQ(2, barrier.GetSourceIndex("C"));
  EXPECT_FALSE(barrier.AllTriggered());
}

TEST_F(GroupBarrierTestSuite, Test_ArriveSource1) { EXPECT_TRUE(TestArriveSource1()); }
TEST_F(GroupBarrierTestSuite, Test_ArriveSource2) { EXPECT_TRUE(TestArriveSource2()); }
TEST_F(GroupBarrierTestSuite, TestReset) { EXPECT_TRUE(TestReset()); }

TEST(GroupBarrierTest, TestArriveSource1) {
  GroupBarrier barrier({{"A", "B"}, {"C", "B"}, {"C"}});
  auto         sourceA = barrier.GetSourceIndex("A");
  auto         sourceB = barrier.GetSourceIndex("B");
  auto         sourceC = barrier.GetSourceIndex("C");
  std::string  output;
  auto         visitor = [&output](const char* data, size_t size) {
    // append data to output
    output.append(data, size);
  };
  {
    auto [triggerInGroup, groupRange] = barrier.ArriveSource(sourceC, visitor, "C", 1);
    EXPECT_TRUE(triggerInGroup);
    EXPECT_EQ("C", output);
    output.clear();
    auto it = groupRange.first;
    EXPECT_EQ("[C]", (*it++)->GetName());
    EXPECT_EQ(groupRange.second, it);
    EXPECT_FALSE(barrier.AllTriggered());
  }
  {
    auto [triggerInGroup, groupRange] = barrier.ArriveSource(sourceB, visitor, "B", 1);
    EXPECT_TRUE(triggerInGroup);
    EXPECT_EQ("BC", output);
    output.clear();
    auto it = groupRange.first;
    EXPECT_EQ("[C, B]", (*it++)->GetName());
    EXPECT_EQ(groupRange.second, it);
    EXPECT_FALSE(barrier.AllTriggered());
  }
  {
    auto [triggerInGroup, groupRange] = barrier.ArriveSource(sourceA, visitor, "A", 1);
    EXPECT_TRUE(triggerInGroup);
    EXPECT_EQ("AB", output);
    output.clear();
    auto it = groupRange.first;
    EXPECT_EQ("[A, B]", (*it++)->GetName());
    EXPECT_EQ(groupRange.second, it);
    EXPECT_TRUE(barrier.AllTriggered());
  }
}

TEST(GroupBarrierTest, TestArriveSource2) {
  GroupBarrier barrier({{"A", "B"}, {"C", "B"}, {"C"}});
  auto         sourceA = barrier.GetSourceIndex("A");
  auto         sourceB = barrier.GetSourceIndex("B");
  auto         sourceC = barrier.GetSourceIndex("C");
  std::string  output;
  auto         visitor = [&output](const char* data, size_t size) {
    // append data to output
    output.append(data, size);
  };
  {
    auto [triggerInGroup, groupRange] = barrier.ArriveSource(sourceA, visitor, "A", 1);
    EXPECT_FALSE(triggerInGroup);
    EXPECT_EQ("", output);
    auto it = groupRange.first;
    EXPECT_EQ(groupRange.second, it);
    EXPECT_FALSE(barrier.AllTriggered());
  }
  {
    auto [triggerInGroup, groupRange] = barrier.ArriveSource(sourceB, visitor, "B", 1);
    EXPECT_TRUE(triggerInGroup);
    EXPECT_EQ("AB", output);
    output.clear();
    auto it = groupRange.first;
    EXPECT_EQ("[A, B]", (*it++)->GetName());
    EXPECT_EQ(groupRange.second, it);
    EXPECT_FALSE(barrier.AllTriggered());
  }
  {
    auto [triggerInGroup, groupRange] = barrier.ArriveSource(sourceC, visitor, "C", 1);
    EXPECT_TRUE(triggerInGroup);
    EXPECT_EQ("BC", output);
    output.clear();
    auto it = groupRange.first;
    EXPECT_EQ("[C, B]", (*it++)->GetName());
    EXPECT_EQ("[C]", (*it++)->GetName());
    EXPECT_EQ(groupRange.second, it);
    EXPECT_TRUE(barrier.AllTriggered());
  }
}

TEST(GroupBarrierTest, TestArriveSource3) {
  GroupBarrier barrier({{"A", "B"}, {"C", "B"}, {"C"}});
  auto         sourceA = barrier.GetSourceIndex("A");
  auto         sourceB = barrier.GetSourceIndex("B");
  auto         sourceC = barrier.GetSourceIndex("C");
  std::string  output;
  auto         visitor = [&output](const char* data, size_t size) {
    // append data to output
    output.append(data, size);
  };
  {
    auto [triggerInGroup, groupRange] = barrier.ArriveSource(sourceB, visitor, "B", 1);
    EXPECT_FALSE(triggerInGroup);
    EXPECT_EQ("", output);
    EXPECT_EQ(groupRange.first, groupRange.second);
    EXPECT_FALSE(barrier.AllTriggered());
  }
  {
    auto [triggerInGroup, groupRange] = barrier.ArriveSource(sourceC, visitor, "C", 1);
    EXPECT_TRUE(triggerInGroup);
    EXPECT_EQ("BC", output);
    output.clear();
    auto it = groupRange.first;
    EXPECT_EQ("[C, B]", (*it++)->GetName());
    EXPECT_EQ("[C]", (*it++)->GetName());
    EXPECT_EQ(groupRange.second, it);
    EXPECT_FALSE(barrier.AllTriggered());
  }
  {
    auto [triggerInGroup, groupRange] = barrier.ArriveSource(sourceA, visitor, "A", 1);
    EXPECT_TRUE(triggerInGroup);
    EXPECT_EQ("AB", output);
    output.clear();
    auto it = groupRange.first;
    EXPECT_EQ("[A, B]", (*it++)->GetName());
    EXPECT_EQ(groupRange.second, it);
    EXPECT_TRUE(barrier.AllTriggered());
  }
}

TEST(GroupBarrierTest, TestArriveSource4) {
  GroupBarrier barrier({{"A", "B"}, {"A", "B", "C"}});
  auto         sourceA = barrier.GetSourceIndex("A");
  auto         sourceB = barrier.GetSourceIndex("B");
  auto         sourceC = barrier.GetSourceIndex("C");
  std::string  output;
  auto         visitor = [&output](const char* data, size_t size) {
    // append data to output
    output.append(data, size);
  };
  {
    auto [triggerInGroup, groupRange] = barrier.ArriveSource(sourceB, visitor, "B", 1);
    EXPECT_FALSE(triggerInGroup);
    EXPECT_EQ("", output);
    EXPECT_EQ(groupRange.first, groupRange.second);
    EXPECT_FALSE(barrier.AllTriggered());
  }
  {
    auto [triggerInGroup, groupRange] = barrier.ArriveSource(sourceC, visitor, "C", 1);
    EXPECT_FALSE(triggerInGroup);
    EXPECT_EQ("", output);
    EXPECT_EQ(groupRange.first, groupRange.second);
    EXPECT_FALSE(barrier.AllTriggered());
  }
  {
    auto [triggerInGroup, groupRange] = barrier.ArriveSource(sourceA, visitor, "A", 1);
    EXPECT_TRUE(triggerInGroup);
    EXPECT_EQ("ABC", output);
    output.clear();
    auto it = groupRange.first;
    EXPECT_EQ("[A, B]", (*it++)->GetName());
    EXPECT_EQ("[A, B, C]", (*it++)->GetName());
    EXPECT_EQ(groupRange.second, it);
    EXPECT_TRUE(barrier.AllTriggered());
  }
}