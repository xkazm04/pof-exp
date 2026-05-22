#include "Physics/ARPGPhysicsConstraintPreset.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"

void UARPGPhysicsConstraintPreset::ApplyToConstraint(UPhysicsConstraintComponent* Constraint) const
{
	if (!Constraint) return;

	switch (ConstraintType)
	{
	case EARPGConstraintType::Hinge:
		Constraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0.f);
		Constraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0.f);
		Constraint->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Limited, TwistLimitAngle);
		Constraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0.f);
		Constraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0.f);
		Constraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0.f);
		break;

	case EARPGConstraintType::BallAndSocket:
		Constraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Limited, Swing1LimitAngle);
		Constraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Limited, Swing2LimitAngle);
		Constraint->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Limited, TwistLimitAngle);
		Constraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0.f);
		Constraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0.f);
		Constraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0.f);
		break;

	case EARPGConstraintType::Prismatic:
		Constraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0.f);
		Constraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0.f);
		Constraint->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0.f);
		Constraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Limited, LinearLimitSize);
		Constraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0.f);
		Constraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0.f);
		break;

	case EARPGConstraintType::Weld:
		Constraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0.f);
		Constraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0.f);
		Constraint->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0.f);
		Constraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0.f);
		Constraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0.f);
		Constraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0.f);
		break;

	case EARPGConstraintType::Custom:
		Constraint->SetAngularSwing1Limit(Swing1LimitAngle > 0.f ? EAngularConstraintMotion::ACM_Limited : EAngularConstraintMotion::ACM_Locked, Swing1LimitAngle);
		Constraint->SetAngularSwing2Limit(Swing2LimitAngle > 0.f ? EAngularConstraintMotion::ACM_Limited : EAngularConstraintMotion::ACM_Locked, Swing2LimitAngle);
		Constraint->SetAngularTwistLimit(TwistLimitAngle > 0.f ? EAngularConstraintMotion::ACM_Limited : EAngularConstraintMotion::ACM_Locked, TwistLimitAngle);
		Constraint->SetLinearXLimit(LinearLimitSize > 0.f ? ELinearConstraintMotion::LCM_Limited : ELinearConstraintMotion::LCM_Locked, LinearLimitSize);
		Constraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0.f);
		Constraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0.f);
		break;
	}
}
