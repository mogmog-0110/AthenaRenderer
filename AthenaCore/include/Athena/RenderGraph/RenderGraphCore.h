#pragma once

// RenderGraphシステムの統合ヘッダー
#include "ResourceHandle.h"
#include "RenderPass.h" 
#include "RenderGraphCore.h"
#include "RenderGraphBuilder.h"

namespace Athena {
    // 便利なusing宣言
    using RenderGraphPtr = std::shared_ptr<class RenderGraph>;
    using RenderPassPtr = std::unique_ptr<class RenderPass>;
}

// 元のRenderGraphクラス定義をRenderGraphCore.hに移動