// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interfaces/IHttpRequest.h"
#include "UGCHttpClient.generated.h"

/**
 * UUGCHttpClient
 *
 * Claude API HTTP 客户端组件，挂载在 PlayerController 上。
 * 封装 Anthropic Messages API 调用，通过 BlueprintNativeEvent
 * 将结果回调给 Lua 层（LLMGateway.lua 覆盖 OnMessageComplete）。
 *
 * 使用方式：
 *   1. 在 BP_FPSPlayerController 的 UGCHttpClient 组件 Details 里填入 APIKey
 *   2. Lua 调用 HttpClient:SendMessage(userText, toolsJSON)
 *   3. Lua 覆盖 OnMessageComplete(responseJSON) 解析结果
 */
UCLASS(ClassGroup = "UGC", meta = (BlueprintSpawnableComponent))
class FPS_API UUGCHttpClient : public UActorComponent
{
    GENERATED_BODY()

public:
    UUGCHttpClient();

    //-------------------------------------------------------------------
    // 配置（在蓝图 Defaults 里填写，不硬编码）
    //-------------------------------------------------------------------

    /** Anthropic API Key（sk-ant-...） */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UGC|LLM|Config")
    FString APIKey;

    /** API 端点，默认 DeepSeek（OpenAI 兼容格式） */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UGC|LLM|Config")
    FString APIEndpoint = TEXT("https://api.deepseek.com/v1/chat/completions");

    /** 使用的模型 ID */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UGC|LLM|Config")
    FString ModelID = TEXT("claude-sonnet-4-6");

    /** 最大输出 Token 数 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UGC|LLM|Config")
    int32 MaxTokens = 1024;

    /** 系统提示词（设定 LLM 角色） */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UGC|LLM|Config",
        meta = (MultiLine = true))
    FString SystemPrompt = TEXT(
        "你是一个游戏内的 UGC 助手，帮助玩家通过自然语言配置游戏规则和角色技能。"
        "请根据用户的描述选择合适的函数调用来实现他们的需求。"
        "回复时优先使用函数调用，只有无法用函数实现时才用文字回复。"
    );

    //-------------------------------------------------------------------
    // 发送请求（Lua 调用）
    //-------------------------------------------------------------------

    /**
     * 向 Claude 发送消息，携带 UGC 函数 Schema
     * @param UserMessage  用户输入的自然语言
     * @param ToolsJSON    UGCFunctionRegistry:GetSchemas() 返回的 JSON 字符串
     */
    UFUNCTION(BlueprintCallable, Category = "UGC|LLM")
    void SendMessage(const FString& UserMessage, const FString& ToolsJSON);

    /** 取消当前进行中的请求 */
    UFUNCTION(BlueprintCallable, Category = "UGC|LLM")
    void CancelRequest();

    /** 是否有请求正在进行 */
    UFUNCTION(BlueprintCallable, Category = "UGC|LLM")
    bool IsRequestInProgress() const { return bRequestInProgress; }

    //-------------------------------------------------------------------
    // 回调（Lua 通过 BlueprintNativeEvent 覆盖）
    //-------------------------------------------------------------------

    /**
     * 请求成功完成
     * @param ResponseJSON  Claude 返回的完整 JSON 字符串，Lua 负责解析
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "UGC|LLM")
    void OnMessageComplete(const FString& ResponseJSON);
    virtual void OnMessageComplete_Implementation(const FString& ResponseJSON);

    /**
     * 请求失败（网络错误 / 非 2xx 状态码）
     * @param ErrorMessage  错误描述
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "UGC|LLM")
    void OnMessageError(const FString& ErrorMessage);
    virtual void OnMessageError_Implementation(const FString& ErrorMessage);

private:
    /** 构建 Claude Messages API 请求体 JSON */
    FString BuildRequestBody(const FString& UserMessage, const FString& ToolsJSON) const;

    /** HTTP 响应回调 */
    void OnHttpResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);

    /** 当前请求是否进行中 */
    bool bRequestInProgress = false;

    /** 当前进行中的请求（用于取消） */
    TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> ActiveRequest;
};
