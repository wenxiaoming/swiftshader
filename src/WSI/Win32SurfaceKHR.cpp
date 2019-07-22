// Copyright 2019 The SwiftShader Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Win32SurfaceKHR.hpp"

#include "Vulkan/VkDeviceMemory.hpp"
#include "Vulkan/VkImage.hpp"

#include <string.h>

namespace vk {

Win32SurfaceKHR::Win32SurfaceKHR(const VkWin32SurfaceCreateInfoKHR *pCreateInfo, void *mem):
	hinstance(pCreateInfo->hinstance),
	hwnd(pCreateInfo->hwnd)
{
	RECT rect;
	GetWindowRect(hwnd, &rect);
	LONG width = rect.right - rect.left;
	LONG height = rect.bottom - rect.top;

	this->width = width;
	this->height = height;

	InitBitmapInfo();
}

void Win32SurfaceKHR::InitBitmapInfo()
{
	hdc = GetDC(hwnd);

	bmpHandle = GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE,
		sizeof(BITMAPINFOHEADER) + (sizeof(RGBQUAD) * 256));
	bmpInfo = (LPBITMAPINFO)GlobalLock(bmpHandle);

	// init bmpInfo
	RGBQUAD *argbq;

	argbqHandle = LocalAlloc(LMEM_ZEROINIT | LMEM_MOVEABLE, (sizeof(RGBQUAD) * 256));
	argbq = (RGBQUAD *)LocalLock(argbqHandle);

	for (int i = 0; i < 256; i++) {
		argbq[i].rgbBlue = i;
		argbq[i].rgbGreen = i;
		argbq[i].rgbRed = i;
		argbq[i].rgbReserved = 0;
	}

	bmpInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmpInfo->bmiHeader.biPlanes = 1;

	bmpInfo->bmiHeader.biBitCount = 32;
	bmpInfo->bmiHeader.biCompression = BI_RGB;
	bmpInfo->bmiHeader.biWidth = width;
	bmpInfo->bmiHeader.biHeight = -height;

	memcpy(bmpInfo->bmiColors, argbq, sizeof(RGBQUAD) * 256);

	LocalUnlock(argbqHandle);
	LocalFree(argbqHandle);
}

void Win32SurfaceKHR::destroySurface(const VkAllocationCallbacks *pAllocator)
{
	ReleaseDC(hwnd, hdc);
	GlobalFree(bmpHandle);
	LocalFree(argbqHandle);
}

size_t Win32SurfaceKHR::ComputeRequiredAllocationSize(const VkWin32SurfaceCreateInfoKHR *pCreateInfo)
{
	return 0;
}

void Win32SurfaceKHR::getSurfaceCapabilities(VkSurfaceCapabilitiesKHR *pSurfaceCapabilities) const
{
	SurfaceKHR::getSurfaceCapabilities(pSurfaceCapabilities);
	RECT rect;
	GetWindowRect(hwnd, &rect);
	LONG width = rect.right - rect.left;
	LONG height = rect.bottom - rect.top;

	VkExtent2D extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

	pSurfaceCapabilities->currentExtent = extent;
	pSurfaceCapabilities->minImageExtent = extent;
	pSurfaceCapabilities->maxImageExtent = extent;
}

void Win32SurfaceKHR::attachImage(PresentImage* image)
{
	VkExtent3D extent = image->getImage()->getMipLevelExtent(VK_IMAGE_ASPECT_COLOR_BIT, 0);

	int bytes_per_line = image->getImage()->rowPitchBytes(VK_IMAGE_ASPECT_COLOR_BIT, 0);
	char* buffer = static_cast<char*>(image->getImageMemory()->getOffsetPointer(0));
}

void Win32SurfaceKHR::detachImage(PresentImage* image)
{

}

void Win32SurfaceKHR::present(PresentImage* image)
{
	RECT rect;
	GetWindowRect(hwnd, &rect);
	LONG width = rect.right - rect.left;
	LONG height = rect.bottom - rect.top;

	if ((this->width != width) || (this->height != height))
	{
		this->width = width;
		this->height = height;

		ReleaseDC(hwnd, hdc);
		GlobalFree(bmpHandle);
		LocalFree(argbqHandle);

		InitBitmapInfo();
	}

	char* buffer = static_cast<char*>(image->getImageMemory()->getOffsetPointer(0));
	SetStretchBltMode(hdc, STRETCH_DELETESCANS);
	StretchDIBits(hdc, 0, 0, width,height,
		0, 0, width, height,
		buffer, bmpInfo, DIB_RGB_COLORS, SRCCOPY);
}

} // namespace vk