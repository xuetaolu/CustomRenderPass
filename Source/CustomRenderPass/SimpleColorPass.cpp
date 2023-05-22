// Fill out your copyright notice in the Description page of Project Settings.


#include "SimpleColorPass.h"

#include "RenderCore/Public/GlobalShader.h"
#include "Engine/TextureRenderTarget2D.h"

class FSimpleColorShader : public FGlobalShader
{
    DECLARE_INLINE_TYPE_LAYOUT(FSimpleColorShader, NonVirtual);

public:

    FSimpleColorShader() {}

    FSimpleColorShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
        : FGlobalShader(Initializer) {
        SimpleColorVal.Bind(Initializer.ParameterMap, TEXT("SimpleColor"));
    }

    static bool ShouldCache(EShaderPlatform Platform) {
        return true;
    }

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
        //return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM4);  
        return true;
    }

    static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
        FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
        // OutEnvironment.SetDefine(TEXT("TEST_MICRO"), 1);
    }

    void SetParameters(
        FRHICommandListImmediate& RHICmdList,
        const FLinearColor& MyColor
    ) const {
        SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), SimpleColorVal, MyColor);
    }

    // 不知道哪个版本之后没有这个方法声明，暂时不用
    // virtual bool Serialize(FArchive& Ar) override {
    //     bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
    //     Ar << SimpleColorVal;
    //     return bShaderHasOutdatedParameters;
    // }

private:
	// 需要使用 LAYOUT_FIELD 定义，否则会提示结构 size 非法。
    LAYOUT_FIELD(FShaderParameter, SimpleColorVal);

};

class FSimpleColorShaderVS : public FSimpleColorShader
{
    DECLARE_SHADER_TYPE(FSimpleColorShaderVS, Global);

public:
    FSimpleColorShaderVS() {}

    FSimpleColorShaderVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
        : FSimpleColorShader(Initializer) {

    }
};

class FSimpleColorShaderPS : public FSimpleColorShader
{
    DECLARE_SHADER_TYPE(FSimpleColorShaderPS, Global);

public:
    FSimpleColorShaderPS() {}

    FSimpleColorShaderPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
        : FSimpleColorShader(Initializer) {

    }
};

IMPLEMENT_SHADER_TYPE(, FSimpleColorShaderVS, TEXT("/Project/SimpleColor.usf"), TEXT("MainVS"), SF_Vertex)
IMPLEMENT_SHADER_TYPE(, FSimpleColorShaderPS, TEXT("/Project/SimpleColor.usf"), TEXT("MainPS"), SF_Pixel)

static void DrawTestShaderRenderTarget_RenderThread(
    FRHICommandListImmediate& RHICmdList,
    FTextureRenderTargetResource* OutputRenderTargetResource,
    ERHIFeatureLevel::Type FeatureLevel,
    FName TextureRenderTargetName,
    FLinearColor MyColor
) {
    check(IsInRenderingThread());

#if WANTS_DRAW_MESH_EVENTS  
    FString EventName;
    TextureRenderTargetName.ToString(EventName);
    SCOPED_DRAW_EVENTF(RHICmdList, SceneCapture, TEXT("ShaderTest %s"), *EventName);
#else  
    SCOPED_DRAW_EVENT(RHICmdList, DrawTestShaderRenderTarget_RenderThread);
#endif  

    FRHITexture2D* RenderTargetTexture = OutputRenderTargetResource->GetRenderTargetTexture();
    RHICmdList.Transition(FRHITransitionInfo(RenderTargetTexture, ERHIAccess::SRVMask, ERHIAccess::RTV));
    FRHIRenderPassInfo RPInfo(RenderTargetTexture, ERenderTargetActions::DontLoad_Store);
    RHICmdList.BeginRenderPass(RPInfo, TEXT("DrawTestShaderRenderTarget"));
    {

        FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(FeatureLevel);
        TShaderMapRef< FSimpleColorShaderVS > VertexShader(GlobalShaderMap);
        TShaderMapRef< FSimpleColorShaderPS > PixelShader(GlobalShaderMap);


        // Set the graphic pipeline state.
        FGraphicsPipelineStateInitializer GraphicsPSOInit;
        RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
        GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
        GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
        GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_CCW>::GetRHI();

        // 使用三角形List 模式，即若用 DrawIndex，需要输入 0 1 2，2 1 3，每3个一组作为三角形
        GraphicsPSOInit.PrimitiveType = PT_TriangleList;

        // 后续设置顶点格式描述
        // GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GetVertexDeclarationFVector4();

        GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
        GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
        SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

        PixelShader->SetParameters(RHICmdList, MyColor);

        // Update viewport.
        RHICmdList.SetViewport(
            0, 0, 0.f,
            OutputRenderTargetResource->GetSizeX(), OutputRenderTargetResource->GetSizeY(), 1.f);


        // 练习，UE 4.27.2 后变化的代码
        struct FTextureVertex
        {
        public:
            FVector4 Position;
            FVector2D UV;
        };

        FVertexDeclarationElementList Elements;
        const auto Stride = sizeof(FTextureVertex);
        Elements.Add(FVertexElement(0, STRUCT_OFFSET(FTextureVertex, Position), VET_Float4, 0, Stride));
        Elements.Add(FVertexElement(0, STRUCT_OFFSET(FTextureVertex, UV), VET_Float2, 1, Stride));
        FVertexDeclarationRHIRef VertexDeclarationRHI = RHICreateVertexDeclaration(Elements);
        // 设置顶点格式描述
        GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = VertexDeclarationRHI;

        SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);


        FRHIResourceCreateInfo CreateInfo;
        FVertexBufferRHIRef VertexBufferRHI = RHICreateVertexBuffer(sizeof(FTextureVertex) * 4, BUF_Volatile, CreateInfo);

        FTextureVertex* Vertices = static_cast<FTextureVertex*>(RHILockVertexBuffer(VertexBufferRHI, 0, sizeof(FTextureVertex) * 4, RLM_WriteOnly));
        Vertices[0].Position = FVector4(-1.0f, 1.0f, 0, 1.0f);
        Vertices[1].Position = FVector4(1.0f, 1.0f, 0, 1.0f);
        Vertices[2].Position = FVector4(-1.0f, -1.0f, 0, 1.0f);
        Vertices[3].Position = FVector4(1.0f, -1.0f, 0, 1.0f);
        Vertices[0].UV = FVector2D(0, 0);
        Vertices[1].UV = FVector2D(1, 0);
        Vertices[2].UV = FVector2D(0, 1);
        Vertices[3].UV = FVector2D(1, 1);
        RHIUnlockVertexBuffer(VertexBufferRHI);

        static const uint16 Indices[6] =
        {
            0, 1, 2,
            2, 1, 3,
        };

        FIndexBufferRHIRef IndexBufferRHI = RHICreateIndexBuffer(sizeof(uint16), sizeof(Indices), BUF_Volatile, CreateInfo);

        uint16* Indexs = static_cast<uint16*>(RHILockIndexBuffer(IndexBufferRHI, 0, sizeof(Indices), RLM_WriteOnly));

        for (int i = 0; i < sizeof(Indices) / sizeof(uint16); ++i) {
            Indexs[i] = Indices[i];
        }

        RHIUnlockIndexBuffer(IndexBufferRHI);

        RHICmdList.SetStreamSource(0, VertexBufferRHI, 0);
        RHICmdList.DrawIndexedPrimitive(IndexBufferRHI, 0, 0, 4, 0, 2, 1);

        // Resolve render target.  
        RHICmdList.CopyToResolveTarget(
            OutputRenderTargetResource->GetRenderTargetTexture(),
            OutputRenderTargetResource->TextureRHI,
            FResolveParams());
    }


    RHICmdList.EndRenderPass();

}


USimpleColorPassBlueprintLibrary::USimpleColorPassBlueprintLibrary(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {

}

void USimpleColorPassBlueprintLibrary::DrawTestShaderRenderTarget(
    UTextureRenderTarget2D* OutputRenderTarget,
    AActor* Ac,
    FLinearColor MyColor
) {
    check(IsInGameThread());

    if (!OutputRenderTarget) {
        return;
    }

    FTextureRenderTargetResource* TextureRenderTargetResource = OutputRenderTarget->GameThread_GetRenderTargetResource();
    UWorld* World = Ac->GetWorld();
    ERHIFeatureLevel::Type FeatureLevel = World->Scene->GetFeatureLevel();
    FName TextureRenderTargetName = OutputRenderTarget->GetFName();
    ENQUEUE_RENDER_COMMAND(CaptureCommand)(
        [TextureRenderTargetResource, FeatureLevel, MyColor, TextureRenderTargetName](FRHICommandListImmediate& RHICmdList) {
        DrawTestShaderRenderTarget_RenderThread(RHICmdList, TextureRenderTargetResource, FeatureLevel, TextureRenderTargetName, MyColor);
    }
    );

}