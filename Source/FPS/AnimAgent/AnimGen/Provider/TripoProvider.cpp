// Copyright Epic Games, Inc. All Rights Reserved.

#include "TripoProvider.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

const TCHAR* FTripoProvider::TaskEndpoint = TEXT("https://api.tripo3d.ai/v2/openapi/task");

void FTripoProvider::CreateTextTo3DTask(
    const FString& ApiKey,
    const FAnimGenRequest& Request,
    FOnAnimGenTaskCreated OnCreated)
{
    if (ApiKey.IsEmpty())
    {
        OnCreated.ExecuteIfBound(false, FString(), TEXT("Tripo API Key 未配置"));
        return;
    }

    TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
    Body->SetStringField(TEXT("type"), TEXT("text_to_model"));
    Body->SetStringField(TEXT("prompt"), Request.Prompt);
    Body->SetNumberField(TEXT("face_limit"), Request.TargetPolycount);
    Body->SetBoolField(TEXT("texture"), Request.bWithTexture);
    Body->SetBoolField(TEXT("pbr"), Request.bPBR);

    FString BodyStr;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyStr);
    FJsonSerializer::Serialize(Body, Writer);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = FHttpModule::Get().CreateRequest();
    Req->SetURL(TaskEndpoint);
    Req->SetVerb(TEXT("POST"));
    Req->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Req->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey));
    Req->SetContentAsString(BodyStr);

    Req->OnProcessRequestComplete().BindLambda(
        [OnCreated](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bOk)
        {
            if (!bOk || !Response.IsValid())
            {
                OnCreated.ExecuteIfBound(false, FString(), TEXT("Tripo 请求失败（网络错误）"));
                return;
            }

            const FString Content = Response->GetContentAsString();
            TSharedPtr<FJsonObject> Json;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Content);
            if (!FJsonSerializer::Deserialize(Reader, Json) || !Json.IsValid())
            {
                OnCreated.ExecuteIfBound(false, FString(), TEXT("Tripo 响应 JSON 解析失败"));
                return;
            }

            const int32 Code = Json->GetIntegerField(TEXT("code"));
            if (Code != 0)
            {
                OnCreated.ExecuteIfBound(false, FString(),
                    FString::Printf(TEXT("Tripo error code=%d"), Code));
                return;
            }

            const TSharedPtr<FJsonObject>* Data = nullptr;
            FString TaskId;
            if (Json->TryGetObjectField(TEXT("data"), Data) && Data && Data->IsValid()
                && (*Data)->TryGetStringField(TEXT("task_id"), TaskId))
            {
                OnCreated.ExecuteIfBound(true, TaskId, FString());
            }
            else
            {
                OnCreated.ExecuteIfBound(false, FString(), TEXT("Tripo 响应缺少 task_id"));
            }
        });

    Req->ProcessRequest();
}

void FTripoProvider::QueryTask(
    const FString& ApiKey,
    const FString& TaskId,
    FOnAnimGenTaskQueried OnQueried)
{
    const FString Url = FString::Printf(TEXT("%s/%s"), TaskEndpoint, *TaskId);

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
                St.ErrorMessage = TEXT("Tripo 查询失败（网络错误）");
                OnQueried.ExecuteIfBound(St);
                return;
            }

            TSharedPtr<FJsonObject> Json;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
            if (!FJsonSerializer::Deserialize(Reader, Json) || !Json.IsValid())
            {
                St.State = EAnimGenState::Failed;
                St.ErrorMessage = TEXT("Tripo 响应 JSON 解析失败");
                OnQueried.ExecuteIfBound(St);
                return;
            }

            const TSharedPtr<FJsonObject>* Data = nullptr;
            if (!Json->TryGetObjectField(TEXT("data"), Data) || !Data || !Data->IsValid())
            {
                St.State = EAnimGenState::Failed;
                St.ErrorMessage = TEXT("Tripo 响应缺少 data");
                OnQueried.ExecuteIfBound(St);
                return;
            }

            const FString Status = (*Data)->GetStringField(TEXT("status"));
            (*Data)->TryGetNumberField(TEXT("progress"), St.Progress);

            if (Status == TEXT("success"))
            {
                St.State = EAnimGenState::Succeeded;
                const TSharedPtr<FJsonObject>* Result = nullptr;
                if ((*Data)->TryGetObjectField(TEXT("result"), Result) && Result && Result->IsValid())
                {
                    const TSharedPtr<FJsonObject>* PbrModel = nullptr;
                    if ((*Result)->TryGetObjectField(TEXT("pbr_model"), PbrModel) && PbrModel && PbrModel->IsValid())
                    {
                        (*PbrModel)->TryGetStringField(TEXT("url"), St.ModelURL);
                    }
                    const TSharedPtr<FJsonObject>* Rendered = nullptr;
                    if ((*Result)->TryGetObjectField(TEXT("rendered_image"), Rendered) && Rendered && Rendered->IsValid())
                    {
                        (*Rendered)->TryGetStringField(TEXT("url"), St.PreviewURL);
                    }
                }
            }
            else if (Status == TEXT("failed") || Status == TEXT("cancelled"))
            {
                St.State = EAnimGenState::Failed;
                St.ErrorMessage = FString::Printf(TEXT("Tripo 任务 %s"), *Status);
            }
            else
            {
                St.State = (St.Progress > 0) ? EAnimGenState::Running : EAnimGenState::Pending;
            }

            OnQueried.ExecuteIfBound(St);
        });

    Req->ProcessRequest();
}
