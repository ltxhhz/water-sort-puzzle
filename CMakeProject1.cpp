#include "CMakeProject1.h"
#include "DFS.h"
#include "resource.h"
// using namespace std;

using std::string;
using namespace Gdiplus;

struct ContourInfo {
  int index;
  std::vector<cv::Point> contour;
  cv::Rect boundingRect;
  std::vector<std::string> colors;
};

struct ColorInfo {
  int r, g, b;
  // int hex;
  std::string hexString;
};

const std::string whitePixelValue = "#edf2fe";
const int blockNum = 4;

static ColorInfo hexToRGB(const std::string &hexColor) {
  ColorInfo rgb;
  if (hexColor[0] != '#')
    throw std::runtime_error("Invalid hex color format");
  rgb.hexString = hexColor;
  rgb.r = std::stoi(hexColor.substr(1, 2), nullptr, 16);
  rgb.g = std::stoi(hexColor.substr(3, 2), nullptr, 16);
  rgb.b = std::stoi(hexColor.substr(5, 2), nullptr, 16);
  return rgb;
}

// 计算两个颜色之间的RGB欧几里得距离
static double colorDistance(ColorInfo color1, ColorInfo color2) {
  int rmean = (color1.r + color2.r) / 2;
  int r = color1.r - color2.r;
  int g = color1.g - color2.g;
  int b = color1.b - color2.b;
  return std::sqrt((2 + rmean / 256) * (r * r) + 4 * (g * g) + (2 + (255 - rmean) / 256) * (b * b));
}
static std::string BGRToHex(const cv::Vec3b &color) {
  std::stringstream ss;
  ss << "#";
  for (int i = 2; i >= 0; --i) { // 逆序输出，转换为RGB顺序
    ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(color[i]);
  }
  return ss.str();
}

static unsigned int hexColorStringToHexInt(const std::string &hexColor) {
  // 确保字符串以#开头且长度为7（包括#）
  if (hexColor.size() != 7 || hexColor[0] != '#') {
    throw std::invalid_argument("Invalid color format. Expected a 6-character hexadecimal color starting with '#'.");
  }

  // 初始化结果变量
  unsigned int result = 0;

  // 遍历从1开始的每个字符（跳过井号），将其转换为16进制数字并累加到结果中
  for (size_t i = 1; i < hexColor.size(); ++i) {
    char c = toupper(hexColor[i]);
    int value;
    if (c >= '0' && c <= '9') {
      value = c - '0';
    } else if (c >= 'A' && c <= 'F') {
      value = 10 + (c - 'A');
    } else {
      throw std::invalid_argument("Invalid character found in color string.");
    }
    // 根据位置计算实际的16进制值并累加
    result = (result << 4) | value; // 左移4位相当于乘以16，然后与当前值相或累加
  }

  return result;
}

// 函数模板声明
template <typename T> typename std::vector<T>::iterator findWithSkip(std::vector<T> &vec, const T &target, size_t skipCount = 0);

// 函数模板定义
template <typename T> typename std::vector<T>::iterator findWithSkip(std::vector<T> &vec, const T &target, size_t skipCount) {
  auto it = vec.begin();
  while (skipCount > 0) {
    it = std::find(it, vec.end(), target);
    if (it == vec.end())
      return it; // 如果找不到更多匹配项，则返回end()
    ++it;        // 移动到下一个元素继续查找
    --skipCount;
  }
  return std::find(it, vec.end(), target);
}

static std::wstring stringToWstring(const std::string &str) {
  std::vector<wchar_t> buf(str.size() + 1);
  mbstowcs(&buf[0], str.c_str(), str.size());
  return std::wstring(&buf[0]);
}

static std::vector<std::string> detectColorsFromMiddleColumn(const cv::Mat &image) {
  //  获取图像的中间列
  int middleCol = image.cols / 2;
  std::vector<std::string> colorsVec;
  std::map<std::string, int> colorsMap; // 改为数组
  // 遍历中间列，从上到下读取像素值
  bool startReading = false;
  for (int i = 0; i < image.rows; i++) {
    std::string pixelValue = BGRToHex(image.at<cv::Vec3b>(i, middleCol));
    colorsVec.push_back(pixelValue);
    if (!colorsMap[pixelValue]) {
      colorsMap[pixelValue] = 0;
    }
    colorsMap[pixelValue]++;
  }
  // 顺序数组去重
  // std::vector<std::string> uniqueVec;
  // std::for_each(colorsVec.begin(), colorsVec.end(), [&](std::string value) {
  //  if (std::find(uniqueVec.begin(), uniqueVec.end(), value) == uniqueVec.end()) {
  //    uniqueVec.push_back(value);
  //  }
  //});
  // colorsVec.swap(uniqueVec);

  // 对数组连续重复的内容进行去重
  colorsVec.erase(std::unique(colorsVec.begin(), colorsVec.end()), colorsVec.end());

  double colorsCount = 0;
  for (auto &item : colorsMap) {
    colorsCount += item.second;
  }
  double colorsCountAvg = colorsCount / colorsMap.size();
  std::vector<std::string> colorsCountMoreThanAvg;
  for (auto &item : colorsMap) {
    if (item.second > colorsCountAvg && colorDistance(hexToRGB(whitePixelValue), hexToRGB(item.first)) > 50) {
      colorsCountMoreThanAvg.push_back(item.first);
    }
  }
  std::sort(colorsCountMoreThanAvg.begin(), colorsCountMoreThanAvg.end(), [&](std::string a, std::string b) { return colorsMap[a] > colorsMap[b]; });

  for (const string color1 : colorsCountMoreThanAvg) {
    auto pos = find(colorsVec.begin(), colorsVec.end(), color1);
    int findNum = 1;
    while (pos != colorsVec.end()) {
      int i = distance(colorsVec.begin(), pos);
      for (int j = i - 1; j >= 0; j--) {
        std::string color2 = colorsVec[j];
        double distance = colorDistance(hexToRGB(color1), hexToRGB(color2));
        if (distance < 50) {
          colorsMap[color1] += colorsMap[color2];
          colorsMap.erase(color2);
          colorsVec.erase(colorsVec.begin() + j);
          i--;
        } else {
          break;
        }
      }
      for (int j = i + 1; j < colorsVec.size(); j++) {
        std::string color2 = colorsVec[j];
        if (color1 == color2) {
          colorsVec.erase(colorsVec.begin() + j);
          j--;
          continue;
        }
        double distance = colorDistance(hexToRGB(color1), hexToRGB(color2));
        if (distance < 50) {
          colorsMap[color1] += colorsMap[color2];
          colorsMap.erase(color2);
          colorsVec.erase(colorsVec.begin() + j);
          j--;
        } else {
          break;
        }
      }
      pos = findWithSkip(colorsVec, color1, findNum++);
    }
  }

  // 遍历数组colorsVec两次，第一次从当前颜色i所在的索引向前遍历直到大于阈值，第二次从当前颜色所在索引向后遍历直到大于阈值，计算当前颜色i与其他颜色之间的距离，如果距离大于阈值，则将colorsMap[i]更小的值添加到更大的值中，并且删除当前颜色i
  // for (int i = 0; i < colorsVec.size(); i++) {
  //   std::string color1 = colorsVec[i];
  //   for (int j = i + 1; j < colorsVec.size(); j++) {
  //     std::string color2 = colorsVec[j];
  //     double distance = colorDistance(hexToRGB(color1), hexToRGB(color2));
  //     if (distance < 50) {
  //       if (colorsMap[color1] < colorsMap[color2]) {
  //         colorsMap[color2] += colorsMap[color1];
  //         colorsMap.erase(color1);
  //         colorsVec.erase(colorsVec.begin() + i);
  //         i--;
  //         break;
  //       } else {
  //         colorsMap[color1] += colorsMap[color2];
  //         colorsMap.erase(color2);
  //         colorsVec.erase(colorsVec.begin() + j);
  //         j--;
  //       }
  //     } else {
  //       break;
  //     }
  //   }
  // }
  int blockCount = 0;
  std::vector<std::string> blockVec;
  std::cout << std::endl << "颜色: 出现次数" << std::endl;
  for (std::string colorItem : colorsVec) {
    std::cout << colorItem << ": " << colorsMap[colorItem] << std::endl;
    // colorItem是否在colorsCountMoreThanAvg中
    if (std::find(colorsCountMoreThanAvg.begin(), colorsCountMoreThanAvg.end(), colorItem) != colorsCountMoreThanAvg.end() && colorDistance(hexToRGB(whitePixelValue), hexToRGB(colorItem)) > 50) {
      if (std::find(blockVec.begin(), blockVec.end(), colorItem) == blockVec.end()) {
        blockCount += colorsMap[colorItem];
      }
      blockVec.push_back(colorItem);
    }
  }
  std::cout << std::endl;

  if (colorsVec.size() < 4) {
    std::cout << "颜色数量小于4，无法判断试管颜色" << std::endl;
    return blockVec;
  }

  double avg = blockCount == 0 ? 0 : blockCount / 4;
  std::cout << "颜色出现次数平均值: " << avg << std::endl;
  for (int i = 0; i < blockVec.size(); i++) {
    double a = colorsMap[blockVec[i]] / avg;
    int a1 = round(a);
    // if (a - static_cast<int>(a) > 0.8) {
    //   a1 = static_cast<int>(a) + 1;
    // }
    //   查找当前值在数组中出现的次数
    int count = std::count(blockVec.begin(), blockVec.end(), blockVec[i]);
    if (a1 > 1 && a1 != count) {
      for (int j = 0; j < a1 - 1; j++) {
        blockVec.insert(blockVec.begin() + i + j + 1, blockVec[i]);
        i++;
      }
    }
  }
  std::reverse(blockVec.begin(), blockVec.end());
  // cv::imshow("Gray Image", image);
  // cv::waitKey(0);
  return blockVec;
}

static cv::Mat getTube(const cv::Mat &image, const std::vector<cv::Point> &contour) {
  // 创建一个掩码图像，用于提取试管内部的区域
  cv::Mat mask = cv::Mat::zeros(image.size(), CV_8UC1);
  cv::drawContours(mask, std::vector<std::vector<cv::Point>>{contour}, -1, cv::Scalar(255), cv::FILLED);

  // 提取试管内部的区域
  cv::Mat tubeROI;
  image.copyTo(tubeROI, mask);
  // cv::imshow("Tube ROI", tubeROI);
  //  提取试管内部区域的边界框
  cv::Rect boundingBox = cv::boundingRect(contour);

  // 获取试管内部的ROI
  cv::Mat tubeRegion = tubeROI(boundingBox);
  return tubeRegion;
}
// 根据轮廓位置排序
static void sortRect(std::vector<ContourInfo> &infos) {
  sort(infos.begin(), infos.end(), [](const ContourInfo &a, const ContourInfo &b) {
    if (std::abs(a.boundingRect.y - b.boundingRect.y) <= 3) {
      return a.boundingRect.x < b.boundingRect.x;
    }
    return a.boundingRect.y < b.boundingRect.y;
  });
  for (int i = 0; i < infos.size(); i++) {
    infos[i].index = i;
  }
}

static std::string getMapMinKey(const std::map<std::string, int> &argMap) {
  if (argMap.empty()) {
    throw std::invalid_argument("The map is empty");
  }
  auto minIt = std::min_element(argMap.begin(), argMap.end(), [](const std::pair<std::string, int> &a, const std::pair<std::string, int> &b) { return a.second < b.second; });
  return minIt->first;
}

static std::string getMapMaxKey(const std::map<std::string, int> &argMap) {
  if (argMap.empty()) {
    throw std::invalid_argument("The map is empty");
  }
  auto maxIt = std::max_element(argMap.begin(), argMap.end(), [](const std::pair<std::string, int> &a, const std::pair<std::string, int> &b) { return a.second < b.second; });
  return maxIt->first;
}

struct ExtraColor {
  std::string wrong;
  std::string right;
  bool ok = true;
};

static ExtraColor getExtraColor(std::map<std::string, int> &argMap) {
  string maxKey = getMapMaxKey(argMap);
  int maxValue = argMap[maxKey];
  string minKey = getMapMinKey(argMap);
  int minValue = argMap[minKey];
  ExtraColor extraColor;
  if (maxValue != minValue) {
    extraColor.ok = false;
    for (const auto &it : argMap) {
      if (it.first == minKey) {
        for (const auto &item : argMap) {
          if (item.second != maxValue && item.first != minKey) {
            if (colorDistance(hexToRGB(minKey), hexToRGB(item.first)) < 50) {
              if (argMap[item.first] < argMap[minKey]) {
                argMap[minKey] += item.second;
                argMap[item.first] = 0;
                extraColor.wrong = item.first;
                extraColor.right = minKey;
              } else {
                argMap[item.first] += argMap[minKey];
                argMap[minKey] = 0;
                extraColor.wrong = minKey;
                extraColor.right = item.first;
              }
              break;
            }
          }
        }
      }
    }
    std::erase_if(argMap, [](const std::pair<std::string, int> &p) { return p.second == 0; });
  }
  return extraColor;
}

static bool isSameValue(const std::map<std::string, int> &argMap) {
  if (argMap.empty()) {
    throw std::invalid_argument("The map is empty");
  }
  auto it = argMap.begin();
  int value = it->second;
  for (const auto &pair : argMap) {
    if (pair.second != value) {
      return false;
    }
  }
  return true;
}

// 传入cv::Mat和轮廓集合std::vector<std::vector<cv::Point>>，把轮廓集合画到Mat中
void drawContours(cv::Mat &img, std::vector<std::vector<cv::Point>> contours) {
  cv::drawContours(img, contours, -1, cv::Scalar(0, 0, 255), 3);
  cv::namedWindow("contours", cv::WINDOW_NORMAL);
  cv::imshow("contours", img);
  cv::waitKey(0);
}

static int start(std::string path, std::string templatePath) {
  cv::Mat image = imread(path, cv::IMREAD_COLOR);
  cv::Mat grayImage = cv::imread(path, cv::IMREAD_GRAYSCALE);
  cv::Mat templ = cv::imread(templatePath, cv::IMREAD_GRAYSCALE);
  if (!image.data || !templ.data) {
    std::cout << "imread fail" << std::endl;
    return -1;
  }
  // 使用 Canny 算法检测边缘
  cv::Mat edges, templEdges;
  cv::Canny(grayImage, edges, 100, 200);
  cv::Canny(templ, templEdges, 100, 200);
  // 查找轮廓
  std::vector<std::vector<cv::Point>> contours, templContours;
  cv::findContours(edges, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
  cv::findContours(templEdges, templContours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
  if (contours.empty() || templContours.empty()) {
    std::cerr << "No contours found in images!" << std::endl;
    return -1;
  }
  std::vector<cv::Point> templContour = templContours[0];

  int i = 0;
  std::vector<ContourInfo> contourInfos;
  std::map<string, int> colorsNum;
  std::map<double, int> matchNum;
  // 遍历检测图像的所有轮廓，并与模板轮廓进行匹配
  for (const auto &contour : contours) {
    double matchValue = cv::matchShapes(contour, templContour, cv::CONTOURS_MATCH_I1, 0.0);
    if (!matchNum[matchValue]) {
      matchNum[matchValue] = 0;
    }
    matchNum[matchValue]++;
    if (matchValue < 0.1) {
      ContourInfo info;
      info.contour = contour;
      info.boundingRect = cv::boundingRect(contour);
      cv::Mat originTube = getTube(image, contour);
      // cv::imshow("origin Tube", originTube);
      std::vector<std::string> vec = detectColorsFromMiddleColumn(originTube);
      for (const auto &color : vec) {
        if (!colorsNum[color]) {
          colorsNum[color] = 0;
        }
        if (colorsNum.find(color) != colorsNum.end()) {
          colorsNum[color]++;
        }
      }
      info.colors = vec;
      contourInfos.push_back(info);
      i++;
      cv::drawContours(image, std::vector<std::vector<cv::Point>>{contour}, -1, cv::Scalar(0, 255, 0), 2);
    }
  }
  if (colorsNum.empty()) {
    std::cout << "No tubes found" << std::endl;
    for (auto &m : matchNum) {
      std::cout << m.first << ": " << m.second << std::endl;
    }
    drawContours(image, contours);
    return -1;
  }
  while (true) {
    if (!isSameValue(colorsNum)) {
      std::cout << "各试管颜色数量不同" << std::endl;
      for (const auto &p : colorsNum) {
        std::cout << p.first << ": " << p.second << std::endl;
      }
      std::cout << std::endl;
    }
    ExtraColor ext = getExtraColor(colorsNum);
    if (ext.ok) {
      break;
    } else if (ext.wrong == "") {
      return -1;
    }
    for (ContourInfo &info : contourInfos) {
      // 在info.colors中寻找ext.wrong，如果找到则替换为ext.right
      for (int j = 0; j < info.colors.size(); j++) {
        if (info.colors[j] == ext.wrong) {
          info.colors[j] = ext.right;
        }
      }
    }
  }

  //  显示结果
  cv::namedWindow("Original Image", cv::WINDOW_NORMAL);
  cv::imshow("Original Image", image);
  // cv::waitKey(0);

  std::cout << std::endl << "匹配到的轮廓计数: " << i << std::endl << std::endl;
  if (i == 0) {
    std::cout << "没有匹配到任何试管" << std::endl;
    return 0;
  }
  sortRect(contourInfos);
  DFS::tubes.clear();
  for (const auto &info : contourInfos) {
    std::cout << "试管" << info.index << "颜色: ";
    std::copy(info.colors.begin(), info.colors.end(), std::ostream_iterator<std::string>(std::cout, " "));
    std::cout << std::endl;
    std::vector<int> intColors;
    std::transform(info.colors.begin(), info.colors.end(), std::back_inserter(intColors), hexColorStringToHexInt);

    DFS::tubes.push_back(intColors);
  }
  DFS::searchSolution();
  DFS::printCurrentState();
  cv::waitKey(0);
  return 0;
}

// int main() {
//   if (!SetCurrentDirectory(L"E:\\work-space\\CMakeProject1")) {
//     std::cerr << "Failed to set current directory";
//     return 1;
//   }
//   std::cout << "current_path: " << std::filesystem::current_path() << std::endl;
//   std::string path = "d:/im.png";
//   std::string templatePath = "template.png";
//   start(path, templatePath);
//   return 0;
//}

std::wstring GetTempFilePath() {
  wchar_t tempPath[MAX_PATH];
  wchar_t tempFile[MAX_PATH];

  // Get the temp path
  if (GetTempPath(MAX_PATH, tempPath) == 0) {
    std::cerr << "Failed to get temp path" << std::endl;
    return L"";
  }

  // Get the temp file name
  if (GetTempFileName(tempPath, L"IMG", 0, tempFile) == 0) {
    std::cerr << "Failed to get temp file name" << std::endl;
    return L"";
  }

  return std::wstring(tempFile); // Ensuring a proper extension
}

// 传入资源id和路径，把资源写入到指定路径
void WriteResourceToFile(int resourceId, const std::wstring &filePath);
void WriteResourceToFile(int resourceId, const std::wstring &filePath) {
  // 获取当前模块的句柄
  HMODULE hModule = GetModuleHandle(NULL);
  if (!hModule) {
    std::wcerr << L"Failed to get module handle." << std::endl;
    return;
  }

  // 查找资源
  HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(resourceId), L"PNG");
  if (!hResource) {
    std::wcerr << L"Failed to find resource." << std::endl;
    return;
  }

  // 加载资源
  HGLOBAL hLoadedResource = LoadResource(hModule, hResource);
  if (!hLoadedResource) {
    std::wcerr << L"Failed to load resource." << std::endl;
    return;
  }

  // 获取资源数据的指针
  LPVOID pResourceData = LockResource(hLoadedResource);
  if (!pResourceData) {
    std::wcerr << L"Failed to lock resource." << std::endl;
    return;
  }

  // 获取资源的大小
  DWORD resourceSize = SizeofResource(hModule, hResource);
  if (resourceSize == 0) {
    std::wcerr << L"Resource size is zero." << std::endl;
    return;
  }

  // 将资源写入文件
  std::ofstream outFile(filePath, std::ios::out | std::ios::binary);
  if (!outFile) {
    std::wcerr << L"Failed to open file for writing." << std::endl;
    return;
  }

  outFile.write(static_cast<const char *>(pResourceData), resourceSize);
  if (!outFile) {
    std::wcerr << L"Failed to write resource to file." << std::endl;
    return;
  }

  outFile.close();
  if (!outFile) {
    std::wcerr << L"Failed to close the file." << std::endl;
    return;
  }

  std::wcout << L"Resource written to file successfully." << std::endl;
}

void AttachConsoleForDebug();
void AttachConsoleForDebug() {
  if (AllocConsole()) {
    // 重定向标准输入
    freopen("conin$", "r", stdin);

    // 重定向标准输出
    freopen("conout$", "w", stdout);
    freopen("conout$", "w", stderr);

    // 将控制台窗口的句柄设为stdout的默认关联句柄
    _setmode(_fileno(stdout), _O_TEXT);
    _setmode(_fileno(stdin), _O_TEXT);
  }
}

std::wstring GetErrorMessage(HRESULT hr) {
  // Check if the HRESULT is a Windows error code
  if (HRESULT_FACILITY(hr) == FACILITY_WINDOWS) {
    hr = HRESULT_CODE(hr);
  }

  wchar_t *errorMsg = nullptr;

  // Try to format the message
  DWORD result = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                               reinterpret_cast<LPWSTR>(&errorMsg), 0, nullptr);

  std::wstring message;
  if (result) {
    message = errorMsg;
    LocalFree(errorMsg);
  } else {
    // If FormatMessage fails, use _com_error for more detail
    _com_error err(hr);
    message = err.ErrorMessage();
  }

  // Handle some specific HRESULT error codes
  if (message.empty()) {
    switch (hr) {
    case E_FAIL:
      message = L"Unspecified failure (E_FAIL)";
      break;
    case E_ACCESSDENIED:
      message = L"General access denied error (E_ACCESSDENIED)";
      break;
    case E_OUTOFMEMORY:
      message = L"Out of memory (E_OUTOFMEMORY)";
      break;
    case E_INVALIDARG:
      message = L"One or more arguments are not valid (E_INVALIDARG)";
      break;
    case E_NOTIMPL:
      message = L"Not implemented (E_NOTIMPL)";
      break;
    case E_POINTER:
      message = L"Invalid pointer (E_POINTER)";
      break;
    default:
      message = L"Unknown error";
      break;
    }
  }

  return message;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void CaptureScreenshotFromUser(HWND hwnd);

static std::string templateTempPath;
static std::string screenshotTempPath;

void DeleteTempFiles();
// 删除临时文件
void DeleteTempFiles() {
  if (std::filesystem::exists(templateTempPath)) {
    std::filesystem::remove(templateTempPath);
    return;
  }
  if (std::filesystem::exists(screenshotTempPath)) {
    std::filesystem::remove(screenshotTempPath);
    return;
  }
}

// 控制台关闭事件处理函数
BOOL WINAPI ConsoleHandler(DWORD signal) {
  if (signal == CTRL_CLOSE_EVENT || signal == CTRL_C_EVENT || signal == CTRL_BREAK_EVENT) {
    std::wcout << L"Console window is closing..." << std::endl;
    // 在此处执行退出时的清理操作
    DeleteTempFiles();
    return TRUE;
  }
  return FALSE;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
  GdiplusStartupInput gdiplusStartupInput;
  ULONG_PTR gdiplusToken;
  GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
  AllocConsole();
  FILE *fp;
  freopen_s(&fp, "CONOUT$", "w", stdout);
  std::wstring tmp = GetTempFilePath();
  std::wstring tmp2 = GetTempFilePath();
  templateTempPath = std::string(tmp.begin(), tmp.end());
  screenshotTempPath = std::string(tmp2.begin(), tmp2.end());
  WriteResourceToFile(TEMPLATE_PNG, tmp);

  const wchar_t CLASS_NAME[] = L"Sample Window Class";

  WNDCLASS wc = {};
  wc.lpfnWndProc = WindowProc;
  wc.hInstance = hInstance;
  wc.lpszClassName = CLASS_NAME;

  RegisterClass(&wc);

  HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"Sample Window", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, nullptr, nullptr, hInstance, nullptr);

  if (hwnd == nullptr) {
    return 0;
  }

  ShowWindow(hwnd, nCmdShow);


  // 注册控制台关闭处理函数
  if (SetConsoleCtrlHandler(ConsoleHandler, TRUE)) {
    std::wcout << L"Control handler installed." << std::endl;
  } else {
    std::wcerr << L"Error: could not set control handler." << std::endl;
    return 1;
  }

  MSG msg = {};
  while (GetMessage(&msg, nullptr, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  FreeConsole();
  GdiplusShutdown(gdiplusToken);
  UnregisterClass(wc.lpszClassName, wc.hInstance);
  DeleteTempFiles();
  return 0;
}
static CImage screenshot;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  static HWND hwndCaptureButton;
  static HWND hwndStartButton;
  static RECT topRect;
  static RECT bottomRect;

  switch (uMsg) {
  case WM_CREATE:
    hwndCaptureButton = CreateWindow(L"BUTTON", L"获取截图", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 200, 510, 100, 30, hwnd, (HMENU)IDC_BUTTON_CAPTURE,
                                     (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), nullptr);

    hwndStartButton = CreateWindow(L"BUTTON", L"开始识别", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 500, 510, 100, 30, hwnd, (HMENU)IDC_BUTTON_START,
                                   (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), nullptr);

    // Define the top and bottom regions
    GetClientRect(hwnd, &topRect);
    topRect.bottom = topRect.bottom * 0.8;
    bottomRect.top = topRect.bottom;
    bottomRect.bottom = bottomRect.top + 120; // Adjust height for bottom part
    break;

  case WM_COMMAND:
    if (LOWORD(wParam) == IDC_BUTTON_CAPTURE) {
      CaptureScreenshotFromUser(hwnd);
    } else if (LOWORD(wParam) == IDC_BUTTON_START) {
      if (!screenshot.IsNull()) {
        SaveClipboardImage(stringToWstring(screenshotTempPath).c_str());
        std::cout << screenshotTempPath << std::endl;
        start(screenshotTempPath, templateTempPath);
      }
    }
    break;

  case WM_PAINT: {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    // Fill top part with a color
    FillRect(hdc, &topRect, (HBRUSH)(COLOR_WINDOW + 1));

    // Draw the screenshot if available
    if (!screenshot.IsNull()) {
      long width = screenshot.GetWidth();
      long height = screenshot.GetHeight();
      long containerWidth = topRect.right - topRect.left;
      long containerHeight = topRect.bottom - topRect.top;
      double ratio = std::max((double)width / containerWidth, (double)height / containerHeight);
      screenshot.Draw(hdc, (containerWidth - width / ratio) / 2, (containerHeight - height / ratio) / 2, width / ratio, height / ratio);
      // screenshot.Draw(hdc, topRect.left, topRect.top, topRect.right - topRect.left, topRect.bottom - topRect.top);
    }

    // Fill bottom part with a color
    FillRect(hdc, &bottomRect, (HBRUSH)(COLOR_3DFACE + 1));

    EndPaint(hwnd, &ps);
  } break;
  case WM_DESTROY:
    PostQuitMessage(0);
    break;

  default:
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
  }
  return 0;
}

void SaveClipboardImage(const wchar_t *filename) {
  // 初始化GDI+
  GdiplusStartupInput gdiplusStartupInput;
  ULONG_PTR gdiplusToken;
  GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

  // 打开剪贴板
  if (OpenClipboard(nullptr)) {
    // 获取剪贴板中的图片数据
    HBITMAP hBitmap = (HBITMAP)GetClipboardData(CF_BITMAP);
    if (hBitmap) {
      // 将HBITMAP转换为GDI+的Bitmap
      Bitmap bitmap(hBitmap, nullptr);

      // 保存图片到文件
      CLSID pngClsid;
      GetEncoderClsid(L"image/png", &pngClsid);
      bitmap.Save(filename, &pngClsid, nullptr);
    } else {
      std::wcerr << L"剪贴板中没有图片数据。" << std::endl;
    }
    // 关闭剪贴板
    CloseClipboard();
  } else {
    std::wcerr << L"无法打开剪贴板。" << std::endl;
  }

  // 关闭GDI+
  GdiplusShutdown(gdiplusToken);
}

// 获取编码器的CLSID
int GetEncoderClsid(const WCHAR *format, CLSID *pClsid) {
  UINT num = 0;
  UINT size = 0;

  GetImageEncodersSize(&num, &size);
  if (size == 0)
    return -1;

  ImageCodecInfo *pImageCodecInfo = (ImageCodecInfo *)(malloc(size));
  if (pImageCodecInfo == nullptr)
    return -1;

  GetImageEncoders(num, size, pImageCodecInfo);

  for (UINT j = 0; j < num; ++j) {
    if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
      *pClsid = pImageCodecInfo[j].Clsid;
      free(pImageCodecInfo);
      return j;
    }
  }
  free(pImageCodecInfo);
  return -1;
}

void CaptureScreenshotFromUser(HWND hwnd) {
  if (OpenClipboard(nullptr)) {
    HBITMAP hBitmap = (HBITMAP)GetClipboardData(CF_BITMAP);
    if (hBitmap) {
      if (GetObjectType(hBitmap) == OBJ_BITMAP) {
        if (!screenshot.IsNull()) {
          HBITMAP hb = screenshot.Detach();
          if (hb != NULL) {
            DeleteObject(hb);
          }
        }

        // Convert the HBITMAP to a CImage
        screenshot.Attach(hBitmap);

        // Save the screenshot to a file
        // screenshot.Save(L"screenshot.png");

        // Display the screenshot in the window
        InvalidateRect(hwnd, nullptr, TRUE);
      }
    }
    CloseClipboard();
  }

  // Redraw the window to display the new screenshot
  InvalidateRect(hwnd, nullptr, TRUE);
}