
#include "DFS.h"
// 算法来源 https://blog.csdn.net/nameofcsdn/article/details/117620445

std::vector<std::vector<int>> DFS::tubes;
std::vector<std::vector<int>> DFS::tubesCopy;
std::map<std::vector<std::vector<int>>, int> DFS::visit;
std::vector<Step> DFS::steps;

bool DFS::searchSolution(int deep) {
  if (deep == 0) {
    visit.clear();
    DFS::steps.clear();
    tubesCopy = vector(tubes);
  }
  if (visit.find(tubes) != visit.end())
    return false;
  visit[tubes] = 1;
  if (isGoalReached() || deep > 50) {
    std::reverse(steps.begin(), steps.end());
    printSteps();
    return true;
  }
  for (int i = 0; i < tubes.size(); i++) {
    for (int j = 0; j < tubes.size(); j++) {
      if (!isTransferPossible(i, j))
        continue;
      int x = transferLiquid(i, j);
      steps.push_back(Step{deep, i, j});
      if (searchSolution(deep + 1)) {
        return true;
      }
      steps.pop_back();
      reverseTransfer(j, i, x);
    }
  }
  return false;
}

void DFS::printSteps(bool prt) {
  cout << endl << "步骤:" << endl;
  for (int i = steps.size() - 1; i >= 0; i--) {
    cout << "step " << steps[i].deep + 1 << ": from " << steps[i].from + 1 << " to " << steps[i].to + 1 << endl;
    if (prt) {
      transferLiquid(steps[i].from, steps[i].to, true);
      printCurrentState(true);
    }
  }
}

void DFS::printCurrentState(bool replay) {
  cout << endl << "当前试管状态:" << endl;
  vector<vector<int>> &tb = replay ? tubesCopy : tubes;
  for (int i = 0; i < tb.size(); i++) {
    cout << "Tube " << i + 1 << ": ";
    for (int j = 0; j < tb[i].size(); j++) {
      cout << tb[i][j] << " ";
    }
    cout << endl;
  }
  cout << endl;
}

bool DFS::isUniformColumn(vector<int> &column) {
  for (int i = 1; i < column.size(); i++)
    if (column[i] != column[i - 1])
      return false;
  return true;
}

bool DFS::isGoalReached() {
  for (auto &vi : tubes)
    if (!isUniformColumn(vi))
      return false;
  return true;
}

bool DFS::isTransferPossible(int fromTube, int toTube, bool replay) {
  if (fromTube == toTube)
    return false;
  vector<vector<int>> &tb = replay ? tubesCopy : tubes;
  int si = tb[fromTube].size(), sj = tb[toTube].size();
  if (si == 0 || sj == SIZE)
    return false;
  if (sj == 0) { // 排除纯色元素倒入空试管的情况
    return !isUniformColumn(tb[fromTube]);
  }
  int ci = tb[fromTube][si - 1], cj = tb[toTube][sj - 1];
  if (ci != cj)
    return false;
  int num = 0;
  for (int k = si - 1; k >= 0; k--)
    if (tb[fromTube][k] == ci)
      num++;
    else
      break;
  return sj + num <= SIZE; // 加了同色必须倒完的限制，提高搜索效率
}

int DFS::transferLiquid(int fromTube, int toTube, bool replay) {
  int x = 0;
  vector<vector<int>> &tb = replay ? tubesCopy : tubes;
  while (isTransferPossible(fromTube, toTube, replay)) {
    auto it = tb[fromTube].end() - 1;
    tb[toTube].emplace_back(*it);
    tb[fromTube].erase(it);
    x++;
  }

  return x;
}

void DFS::reverseTransfer(int fromTube, int toTube, int amount) {
  while (amount--) {
    auto it = tubes[fromTube].end() - 1;
    tubes[toTube].emplace_back(*it);
    tubes[fromTube].erase(it);
  }
}