#include "ARPGGameMode.h"
#include "Player/ARPGPlayerCharacter.h"
#include "Player/ARPGPlayerController.h"

AARPGGameMode::AARPGGameMode()
{
	DefaultPawnClass = AARPGPlayerCharacter::StaticClass();
	PlayerControllerClass = AARPGPlayerController::StaticClass();
}
