# Compile review of the holding tree (a reviewer agent, 2026-10-20; nothing built or edited)

Definite errors:
1. ALooterCharacter incomplete (C2440): FEmberContext::Character is ALooterCharacter*, forward-declared only in
   Embers/EmberPower.h. Converting it to ACharacter* fails at EmberPowerBase.cpp:209 (BeginUse), EmberFiftyTwoPickup.cpp:40,
   EmberGraveWard.cpp:63,68, EmberIronHide.cpp:32, EmberLastRites.cpp:46,51, EmberLongShot.cpp:27, EmberRailSpike.cpp:83,
   EmberSlagBomb.cpp:106,111, EmberSpyglass.cpp:47, EmberTollChain.cpp:44,60, EmberTripwire.cpp:118,123.
   Fix: #include "Core/LooterCharacter.h" in EmberPowerBase.cpp and those 10 power .cpps (as EmberDustDevil.cpp:6).
2. AWeaponBase incomplete: EmberDoubleOrNothing.cpp:97,100 and EmberLuckyStreak.cpp:109,112 compare/assign
   Shot.Weapon.Get() (AWeaponBase*) against TWeakObjectPtr<AActor> LastPullWeapon. Fix: #include "Weapons/WeaponBase.h"
   in both, or make LastPullWeapon a TWeakObjectPtr<AWeaponBase> (EmberDoubleOrNothing.h:73, EmberLuckyStreak.h:99).
Only builds through the Engine shared PCH (breaks "every .cpp includes what it uses"):
3. AActor undeclared in EmberPowerBase.h:53,57,80,90 (and power headers relying on it; EmberIronHide.h:54's UFUNCTION).
   Fix: class AActor; in EmberPowerBase.h.
4. EmberFX.h:87 FEmberTint::SetMaterial assigns a raw pointer inline to TWeakObjectPtr<UMaterialInstanceDynamic>:
   move the body into EmberFX.cpp.
Nits and logic:
- EmberTollChain.h:74's own bRunning hides UEmberPower's private bRunning (IsRunning vs IsThrowing read different
  flags): rename (bChainOut).
- EmberMarkWidget.h:39 SetWords(const FString& Words), EmberMarkActor.h:52 MarkPoint(const AActor& Target): parameter
  names match members in the declarations only (optional rename).
- HeroTests.cpp:461 (Refused) and :205 (HasAllPowerCode) pass only after create_ember_assets.py sets the PowerClasses.
- UEmberSlagBomb::End(false) while the bomb flies: Burst then EndPool at once, so that pool never burns (only when the
  flight outlasts the 7 s duration).
- Blank rounds still broadcast OnBulletHit with 0 damage (DoubleOrNothing's test relies on it; BulletSubsystem.h's comment
  says a blank "tells no hit": fix the comment or the behaviour).
Checked fine: UHT, overrides, delegates, every project and engine function used, containers, format strings, shadowing,
test overloads.
