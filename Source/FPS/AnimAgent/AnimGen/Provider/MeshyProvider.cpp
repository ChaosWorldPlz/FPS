// Copyright Epic Games, Inc. All Rights Reserved.

#include "MeshyProvider.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

const TCHAR* FMeshyProvider::EndpointBase = TEXT("https://api.meshy.ai/openapi/v2/text-to-3d");

FString FMeshyProvider::StyleToString(EAnimGenStyle Style)
{
    switch (Style)
    {
        case EAnimGenStyle::Cartoon:   return TEXT("cartoon");
        case EAnimGenStyle::Sculpture: return TEXT("sculpture");
        case EAnimGenStyle::Realistic:
        default:                        return TEXT("realistic");
    }
}

void FMeshyProvider::CreateTextTo3DTask(
    const FString& ApiKey,
    const FAnimGenRequest& Request,
    FOnAnimGenTaskCreated OnCreated)
{
    if (ApiKey.IsEmpty())
    {
        OnCreated.ExecuteIfBound(false, FString(), TEXT("Meshy API Key 未配置"));
        return;
    }

    TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
    Body->SetStringField(TEXT("mode"), TEXT("preview"));
    Body->SetStringField(TEXT("prompt"), Request.Prompt);
    Body->SetStringField(TEXT("art_style"), StyleToString(Request.Style));
    if (!Request.NegativePrompt.IsEmpty())
    {
        Body->SetStringField(TEXT("negative_prompt"), Request.NegativePrompt);
    }

    FString BodyStr;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyStr);
    FJsonSerializer::Serialize(Body, Writer);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = FHttpModule::Get().CreateRequest();
    Req->SetURL(EndpointBase);
    Req->SetVerb(TEXT("POST"));
    Req->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Req->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey));
    Req->SetContentAsString(BodyStr);

    Req->OnProcessRequestComplete().BindLambda(
        [OnCreated](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bOk)
        {
            if (!bOk || !Response.IsValid())
            {
                OnCreated.ExecuteIfBound(false, FString(), TEXT("Meshy 请求失败（网络错误）"));
                return;
            }

            const FString Content = Response->GetContentAsString();
            const int32 Status = Response->GetResponseCode();
            if (Status < 200 || Status >= 300)
            {
                OnCreated.ExecuteIfBound(false, FString(),
                    FString::Printf(TEXT("Meshy HTTP %d: %s"), Status, *Content));
                return;
            }

            TSharedPtr<FJsonObject> Json;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Content);
            if (!FJsonSerializer::Deserialize(Reader, Json) || !Json.IsValid())
            {
                OnCreated.ExecuteIfBound(false, FString(), TEXT("Meshy 响应 JSON 解析失败"));
                return;
            }

            FString TaskId;
            if (Json->TryGetStringField(TEXT("result"), TaskId))
            {
                OnCreated.ExecuteIfBound(true, TaskId, FString());
            }
            else
            {
                OnCreated.ExecuteIfBound(false, FString(), TEXT("Meshy 响应缺少 result 字段"));
            }
        });

    Req->ProcessRequest();
}

void FMeshyProvider::QueryTask(
    const FString& ApiKey,
    const FString& TaskId,
    FOnAnimGenTaskQueried OnQueried)
{
    const FString Url = FString::Printf(TEXT("%s/%s"), EndpointBase, *TaskId);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = FHttpModule::Get().CreateRequest();
    Req->SetURL(Url);
    Req->SetVerb(TEXT("GET"));
    Req->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey));

    Req->OnProcessRequestComplete().BindLambda(
        [OnQueried](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bOk)
        {
            FAnimGenJobStatus St;
            if (!bOk || !Response.IsValid())
            {
                St.State = EAnimGenState::Failed;
                St.ErrorMessage = TEXT("Meshy 查询失败（网络错误）");
                OnQueried.ExecuteIfBound(St);
                return;
            }

            TSharedPtr<FJsonObject> Json;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
            if (!FJsonSerializer::Deserialize(Reader, Json) || !Json.IsValid())
            {
                St.State = EAnimGenState::Failed;
                St.ErrorMessage = TEXT("Meshy 响应 JSON 解析失败");
                OnQueried.ExecuteIfBound(St);
                return;
            }

            const FString Status = Json->GetStringField(TEXT("status"));
            St.Progress = Json->GetIntegerField(TEXT("progress"));
            Json->TryGetStringField(TEXT("thumbnail_url"), St.PreviewURL);

            if (Status == TEXT("SUCCEEDED"))
            {
                St.State = EAnimGenState::Succeeded;
                const TSharedPtr<FJsonObject>* ModelUrls = nullptr;
                if (Json->TryGetObjectField(TEXT("model_urls"), ModelUrls) && ModelUrls && ModelUrls->IsValid())
                {
                    (*ModelUrls)->TryGetStringField(TEXT("glb"), St.ModelURL);
                }
            }
            else if (Status == TEXT("FAILED"))
            {
                St.State = EAnimGenState::Failed;
                const TSharedPtr<FJsonObject>* ErrObj = nullptr;
                if (Json->TryGetObjectField(TEXT("task_error"), ErrObj) && ErrObj && ErrObj->IsValid())
                {
                    (*ErrObj)->TryGetStringField(TEXT("message"), St.ErrorMessage);
                }
            }
            else
            {
                St.State = (St.Progress > 0) ? EAnimGenState::Running : EAnimGenState::Pending;
            }

            OnQueried.ExecuteIfBound(St);
        });

    Req->ProcessRequest();
}
