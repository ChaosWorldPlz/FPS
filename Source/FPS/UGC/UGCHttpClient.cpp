// Copyright Epic Games, Inc. All Rights Reserved.

#include "UGCHttpClient.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

UUGCHttpClient::UUGCHttpClient()
{
    PrimaryComponentTick.bCanEverTick = false;
}

// -----------------------------------------------------------------------
// 发送请求
// -----------------------------------------------------------------------

void UUGCHttpClient::SendMessage(const FString& UserMessage, const FString& ToolsJSON)
{
    if (APIKey.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[UGCHttpClient] APIKey 未配置，请在蓝图 Defaults 中填写"));
        OnMessageError(TEXT("APIKey 未配置"));
        return;
    }

    if (bRequestInProgress)
    {
        UE_LOG(LogTemp, Warning, TEXT("[UGCHttpClient] 上一个请求尚未完成"));
        return;
    }

    FString Body = BuildRequestBody(UserMessage, ToolsJSON);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(APIEndpoint);
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"),       TEXT("application/json"));
    Request->SetHeader(TEXT("x-api-key"),          APIKey);
    Request->SetHeader(TEXT("anthropic-version"),  TEXT("2023-06-01"));
    Request->SetContentAsString(Body);

    Request->OnProcessRequestComplete().BindUObject(this, &UUGCHttpClient::OnHttpResponse);

    if (Request->ProcessRequest())
    {
        ActiveRequest = Request;
        bRequestInProgress = true;
        UE_LOG(LogTemp, Log, TEXT("[UGCHttpClient] 请求已发送: %s"), *UserMessage);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[UGCHttpClient] 请求发送失败"));
        OnMessageError(TEXT("请求发送失败"));
    }
}

void UUGCHttpClient::CancelRequest()
{
    if (ActiveRequest.IsValid())
    {
        ActiveRequest->CancelRequest();
        ActiveRequest.Reset();
    }
    bRequestInProgress = false;
}

// -----------------------------------------------------------------------
// 构建请求体
// -----------------------------------------------------------------------

FString UUGCHttpClient::BuildRequestBody(const FString& UserMessage, const FString& ToolsJSON) const
{
    // 手动拼接 JSON，避免复杂的 JsonObject 嵌套操作
    // 结构：{ model, max_tokens, system, tools: [...], messages: [{role,content}] }

    FString SafeUserMsg = UserMessage.Replace(TEXT("\""), TEXT("\\\""));
    FString SafeSystem  = SystemPrompt.Replace(TEXT("\""), TEXT("\\\""));

    FString Body = FString::Printf(
        TEXT("{")
        TEXT("\"model\":\"%s\",")
        TEXT("\"max_tokens\":%d,")
        TEXT("\"system\":\"%s\",")
        TEXT("\"tools\":%s,")
        TEXT("\"messages\":[{\"role\":\"user\",\"content\":\"%s\"}]")
        TEXT("}"),
        *ModelID,
        MaxTokens,
        *SafeSystem,
        ToolsJSON.IsEmpty() ? TEXT("[]") : *ToolsJSON,
        *SafeUserMsg
    );

    return Body;
}

// -----------------------------------------------------------------------
// HTTP 响应处理
// -----------------------------------------------------------------------

void UUGCHttpClient::OnHttpResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
{
    bRequestInProgress = false;
    ActiveRequest.Reset();

    if (!bSuccess || !Response.IsValid())
    {
        FString Err = TEXT("网络请求失败");
        UE_LOG(LogTemp, Error, TEXT("[UGCHttpClient] %s"), *Err);
        OnMessageError(Err);
        return;
    }

    int32 StatusCode = Response->GetResponseCode();
    FString ResponseBody = Response->GetContentAsString();

    UE_LOG(LogTemp, Log, TEXT("[UGCHttpClient] 响应状态: %d"), StatusCode);

    if (StatusCode < 200 || StatusCode >= 300)
    {
        FString Err = FString::Printf(TEXT("HTTP %d: %s"), StatusCode, *ResponseBody);
        UE_LOG(LogTemp, Error, TEXT("[UGCHttpClient] %s"), *Err);
        OnMessageError(Err);
        return;
    }

    // 成功：将完整 JSON 传给 Lua 解析
    OnMessageComplete(ResponseBody);
}

// -----------------------------------------------------------------------
// BlueprintNativeEvent 默认实现（Lua 会覆盖）
// -----------------------------------------------------------------------

void UUGCHttpClient::OnMessageComplete_Implementation(const FString& ResponseJSON)
{
    UE_LOG(LogTemp, Log, TEXT("[UGCHttpClient] OnMessageComplete（C++ 默认，应由 Lua 覆盖）"));
}

void UUGCHttpClient::OnMessageError_Implementation(const FString& ErrorMessage)
{
    UE_LOG(LogTemp, Warning, TEXT("[UGCHttpClient] OnMessageError: %s"), *ErrorMessage);
}
