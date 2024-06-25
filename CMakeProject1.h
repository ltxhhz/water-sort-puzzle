// CMakeProject1.h: 标准系统包含文件的包含文件
// 或项目特定的包含文件。

#pragma once

#include <Windows.h>
#include <io.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <comdef.h>
#include <fcntl.h>
#include <iterator>
#include <opencv2/core/mat.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>

#include <gdiplus.h>
#include <atlimage.h>

#define IDC_BUTTON_CAPTURE 1001
#define IDC_BUTTON_START 1002


void SaveClipboardImage(const wchar_t *filename);
int GetEncoderClsid(const WCHAR *format, CLSID *pClsid);