#ifndef DFS_H
#define DFS_H

#include <iostream>
#include <map>
#include <vector>
using std::vector;
using std::map;
using std::endl;
using std::cout;

struct Step {
  int deep;
  int from;
  int to;
};

class DFS {
public:
  static vector<vector<int>> tubes; // 试管
  static bool searchSolution(int deep = 0);
  static void printCurrentState(bool replay = false);
  static void printSteps(bool printSteps = false);

private:
  static vector<vector<int>> tubesCopy;
  static map<vector<vector<int>>, int> visit;
  static const int SIZE = 4;
  static vector<Step> steps;

  // 是否纯色
  static bool isUniformColumn(vector<int> &column);

  static bool isGoalReached();

  // 能否转移
  static bool isTransferPossible(int fromTube, int toTube, bool replay = false);

  // 转移液体，返回倒了几个
  static int transferLiquid(int fromTube, int toTube, bool replay = false);

  // 按照数目回溯
  static void reverseTransfer(int fromTube, int toTube, int amount);
};

#endif