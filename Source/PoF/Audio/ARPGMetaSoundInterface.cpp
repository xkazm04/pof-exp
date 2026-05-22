#include "Audio/ARPGMetaSoundInterface.h"
#include "Components/AudioComponent.h"

UARPGMetaSoundInterface::UARPGMetaSoundInterface()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UARPGMetaSoundInterface::BeginPlay()
{
	Super::BeginPlay();

	if (!MetaSoundSource)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	AudioComp = NewObject<UAudioComponent>(Owner, TEXT("MetaSoundAudio"));
	if (AudioComp)
	{
		AudioComp->SetSound(MetaSoundSource);
		AudioComp->SetVolumeMultiplier(BaseVolume);
		AudioComp->bAutoActivate = false;
		AudioComp->bAutoDestroy = false;
		AudioComp->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		AudioComp->RegisterComponent();

		ApplyDefaultParameters();

		if (bAutoPlay)
		{
			AudioComp->Play();
		}
	}
}

void UARPGMetaSoundInterface::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AudioComp && AudioComp->IsPlaying())
	{
		AudioComp->Stop();
	}
	Super::EndPlay(EndPlayReason);
}

void UARPGMetaSoundInterface::PlayMetaSound()
{
	if (AudioComp)
	{
		ApplyDefaultParameters();
		AudioComp->Play();
	}
}

void UARPGMetaSoundInterface::StopMetaSound(float FadeOutTime)
{
	if (AudioComp && AudioComp->IsPlaying())
	{
		if (FadeOutTime > 0.f)
		{
			AudioComp->FadeOut(FadeOutTime, 0.f);
		}
		else
		{
			AudioComp->Stop();
		}
	}
}

void UARPGMetaSoundInterface::SetFloatParameter(FName ParamName, float Value)
{
	if (AudioComp)
	{
		AudioComp->SetFloatParameter(ParamName, Value);
	}
}

void UARPGMetaSoundInterface::SetIntParameter(FName ParamName, int32 Value)
{
	if (AudioComp)
	{
		AudioComp->SetIntParameter(ParamName, Value);
	}
}

void UARPGMetaSoundInterface::SetBoolParameter(FName ParamName, bool Value)
{
	if (AudioComp)
	{
		AudioComp->SetBoolParameter(ParamName, Value);
	}
}

void UARPGMetaSoundInterface::TriggerParameter(FName ParamName)
{
	if (AudioComp)
	{
		AudioComp->SetTriggerParameter(ParamName);
	}
}

bool UARPGMetaSoundInterface::IsPlaying() const
{
	return AudioComp && AudioComp->IsPlaying();
}

void UARPGMetaSoundInterface::ApplyDefaultParameters()
{
	if (!AudioComp)
	{
		return;
	}

	for (const FMetaSoundParameterBinding& Binding : ParameterBindings)
	{
		switch (Binding.ParameterType)
		{
		case EMetaSoundParameterType::Float:
			AudioComp->SetFloatParameter(Binding.ParameterName, Binding.DefaultFloat);
			break;
		case EMetaSoundParameterType::Int:
			AudioComp->SetIntParameter(Binding.ParameterName, Binding.DefaultInt);
			break;
		case EMetaSoundParameterType::Bool:
			AudioComp->SetBoolParameter(Binding.ParameterName, Binding.DefaultBool);
			break;
		default:
			break;
		}
	}
}
