/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Tencent is pleased to support the open source community by making libpag available.
//
//  Copyright (C) 2021 THL A29 Limited, a Tencent company. All rights reserved.
//
//  Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//  except in compliance with the License. You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  unless required by applicable law or agreed to in writing, software distributed under the
//  license is distributed on an "as is" basis, without warranties or conditions of any kind,
//  either express or implied. see the license for the specific language governing permissions
//  and limitations under the license.
//
/////////////////////////////////////////////////////////////////////////////////////////////////

#include "TestUtils.h"
#include "framework/pag_test.h"
#include "framework/utils/PAGTestUtils.h"
#include "gpu/opengl/GLDevice.h"
#include "gpu/opengl/GLUtil.h"
#include "rendering/Drawable.h"

namespace pag {
using namespace tgfx;

PAG_TEST_CASE(PAGBlendTest)

/**
 * 用例描述: 测试基础混合模式
 */
PAG_TEST_F(PAGBlendTest, Blend) {
  std::vector<std::string> files;
  GetAllPAGFiles("../resources/blend", files);
  auto pagSurface = PAGSurface::MakeOffscreen(400, 400);
  auto pagPlayer = std::make_shared<PAGPlayer>();
  pagPlayer->setSurface(pagSurface);
  for (auto& file : files) {
    auto pagFile = PAGFile::Load(file);
    EXPECT_NE(pagFile, nullptr);
    pagPlayer->setComposition(pagFile);
    pagPlayer->setMatrix(Matrix::I());
    pagPlayer->setProgress(0.5);
    pagPlayer->flush();
    auto found = file.find_last_of("/\\");
    auto suffixIndex = file.rfind('.');
    auto fileName = file.substr(found + 1, suffixIndex - found - 1);
    EXPECT_TRUE(Baseline::Compare(pagSurface, "PAGBlendTest/Blend_" + fileName));
  }
}

tgfx::GLTextureInfo GetBottomLeftImage(std::shared_ptr<Device> device, int width, int height) {
  auto context = device->lockContext();
  auto gl = GLContext::Unwrap(context);
  tgfx::GLTextureInfo textureInfo;
  CreateGLTexture(gl, width, height, &textureInfo);
  tgfx::BackendTexture backendTexture(textureInfo, width, height);
  auto surface = PAGSurface::MakeFrom(ToPAG(backendTexture), ImageOrigin::BottomLeft);
  device->unlock();
  auto composition = PAGComposition::Make(1080, 1920);
  auto imageLayer = PAGImageLayer::Make(1080, 1920, 1000000);
  imageLayer->replaceImage(PAGImage::FromPath("../assets/mountain.jpg"));
  composition->addLayer(imageLayer);
  auto player = std::make_shared<PAGPlayer>();
  player->setSurface(surface);
  player->setComposition(composition);
  player->flush();
  return textureInfo;
}

/**
 * 用例描述: 使用blend时，如果当前的frameBuffer是BottomLeft的，复制逻辑验证
 */
PAG_TEST_F(PAGBlendTest, CopyDstTexture) {
  auto width = 400;
  auto height = 400;
  auto device = GLDevice::Make();
  auto context = device->lockContext();
  ASSERT_TRUE(context != nullptr);
  auto gl = GLContext::Unwrap(context);
  tgfx::GLTextureInfo textureInfo;
  CreateGLTexture(gl, width, height, &textureInfo);
  auto backendTexture = tgfx::BackendTexture(textureInfo, width, height);
  auto pagSurface = PAGSurface::MakeFrom(ToPAG(backendTexture), ImageOrigin::BottomLeft);
  device->unlock();
  auto pagPlayer = std::make_shared<PAGPlayer>();
  pagPlayer->setSurface(pagSurface);
  auto pagFile = PAGFile::Load("../resources/blend/Multiply.pag");
  pagPlayer->setComposition(pagFile);
  pagPlayer->setMatrix(Matrix::I());
  pagPlayer->setProgress(0.5);
  pagPlayer->flush();
  EXPECT_TRUE(Baseline::Compare(pagSurface, "PAGBlendTest/CopyDstTexture"));

  context = device->lockContext();
  gl = GLContext::Unwrap(context);
  gl->deleteTextures(1, &textureInfo.id);
  device->unlock();
}

/**
 * 用例描述: 替换的texture是BottomLeft，renderTarget是TopLeft
 */
PAG_TEST_F(PAGBlendTest, TextureBottomLeft) {
  auto width = 720;
  auto height = 1280;
  auto device = GLDevice::Make();
  auto replaceTextureInfo = GetBottomLeftImage(device, width, height);
  auto context = device->lockContext();
  ASSERT_TRUE(context != nullptr);
  auto gl = GLContext::Unwrap(context);
  auto backendTexture = tgfx::BackendTexture(replaceTextureInfo, width, height);
  auto replaceImage = PAGImage::FromTexture(ToPAG(backendTexture), ImageOrigin::BottomLeft);
  tgfx::GLTextureInfo textureInfo;
  CreateGLTexture(gl, width, height, &textureInfo);
  backendTexture = tgfx::BackendTexture(textureInfo, width, height);
  auto pagSurface = PAGSurface::MakeFrom(ToPAG(backendTexture), ImageOrigin::TopLeft);
  device->unlock();

  auto pagPlayer = std::make_shared<PAGPlayer>();
  pagPlayer->setSurface(pagSurface);
  auto pagFile = PAGFile::Load("../resources/apitest/texture_bottom_left.pag");
  pagFile->replaceImage(3, replaceImage);
  pagPlayer->setComposition(pagFile);
  pagPlayer->setMatrix(Matrix::I());
  pagPlayer->setProgress(0.34);
  pagPlayer->flush();
  EXPECT_TRUE(Baseline::Compare(pagSurface, "PAGBlendTest/TextureBottomLeft"));

  context = device->lockContext();
  gl = GLContext::Unwrap(context);
  gl->deleteTextures(1, &replaceTextureInfo.id);
  gl->deleteTextures(1, &textureInfo.id);
  device->unlock();
}

/**
 * 用例描述: 替换的texture和renderTarget都是BottomLeft
 */
PAG_TEST_F(PAGBlendTest, BothBottomLeft) {
  auto width = 720;
  auto height = 1280;
  auto device = GLDevice::Make();
  auto replaceTextureInfo = GetBottomLeftImage(device, width, height);
  auto context = device->lockContext();
  ASSERT_TRUE(context != nullptr);
  auto backendTexture = tgfx::BackendTexture(replaceTextureInfo, width / 2, height / 2);
  auto replaceImage = PAGImage::FromTexture(ToPAG(backendTexture), ImageOrigin::BottomLeft);
  replaceImage->setMatrix(
      Matrix::MakeTrans(static_cast<float>(width) * 0.1, static_cast<float>(height) * 0.2));
  auto gl = GLContext::Unwrap(context);
  tgfx::GLTextureInfo textureInfo;
  CreateGLTexture(gl, width, height, &textureInfo);
  backendTexture = tgfx::BackendTexture(textureInfo, width, height);
  auto pagSurface = PAGSurface::MakeFrom(ToPAG(backendTexture), ImageOrigin::BottomLeft);
  device->unlock();

  auto composition = PAGComposition::Make(width, height);
  auto imageLayer = PAGImageLayer::Make(width, height, 1000000);
  imageLayer->replaceImage(replaceImage);
  composition->addLayer(imageLayer);

  auto pagPlayer = std::make_shared<PAGPlayer>();
  pagPlayer->setSurface(pagSurface);
  pagPlayer->setComposition(composition);
  pagPlayer->setMatrix(Matrix::I());
  pagPlayer->setProgress(0.3);
  pagPlayer->flush();
  EXPECT_TRUE(Baseline::Compare(pagSurface, "PAGBlendTest/BothBottomLeft"));

  context = device->lockContext();
  ASSERT_TRUE(context != nullptr);
  gl = GLContext::Unwrap(context);
  gl->deleteTextures(1, &replaceTextureInfo.id);
  gl->deleteTextures(1, &textureInfo.id);
  device->unlock();
}
}  // namespace pag
