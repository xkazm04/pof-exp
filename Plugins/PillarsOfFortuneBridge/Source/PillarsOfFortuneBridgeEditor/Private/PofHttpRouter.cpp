#include "PofHttpRouter.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"

TSharedPtr<FJsonObject> FPofHttpRouter::ParseJsonBody(const TArray<uint8>& Body)
{
    if (Body.Num() == 0)
    {
        return nullptr;
    }

    // Convert body bytes to string
    const FUTF8ToTCHAR Converter(reinterpret_cast<const ANSICHAR*>(Body.GetData()), Body.Num());
    FString BodyStr(Converter.Length(), Converter.Get());

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(BodyStr);

    if (FJsonSerializer::Deserialize(Reader, JsonObject))
    {
        return JsonObject;
    }

    return nullptr;
}

FString FPofHttpRouter::GetQueryParam(const FString& QueryString, const FString& ParamName)
{
    FString SearchKey = ParamName + TEXT("=");
    int32 StartIndex = QueryString.Find(SearchKey);
    if (StartIndex == INDEX_NONE)
    {
        return FString();
    }

    StartIndex += SearchKey.Len();
    int32 EndIndex = QueryString.Find(TEXT("&"), ESearchCase::IgnoreCase, ESearchDir::FromStart, StartIndex);
    if (EndIndex == INDEX_NONE)
    {
        EndIndex = QueryString.Len();
    }

    return QueryString.Mid(StartIndex, EndIndex - StartIndex);
}
