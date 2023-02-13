//Copyright Jarrad Alexander 2022


#include "WorldCursorComponent.h"

// Sets default values for this component's properties
UWorldCursorComponent::UWorldCursorComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UWorldCursorComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void UWorldCursorComponent::EndPlay(EEndPlayReason::Type Reason)
{
	//End all captures.
	while (Delegates.Num() > 0)
		RemoveDelegate({ this, Delegates[0].Key });
	

	Super::EndPlay(Reason);
}


// Called every frame
void UWorldCursorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

bool UWorldCursorComponent::IsValidDelegateHandle(const FWorldCursorDelegateHandle& Handle)
{
	if (!IsValid(Handle.GetCursor()) || Handle.GetID() < 0)
		return false;

	for (auto& [ID, Delegate] : Handle.GetCursor()->Delegates)
		if (ID == Handle.GetID())
			return true;

	return false;
}

FWorldCursorDelegateHandle UWorldCursorComponent::AddDelegate(const FWorldCursorEventDelegate& Delegate)
{
	auto Handle = NewDelegateHandle();

	Delegates.Insert({ Handle.GetID(), Delegate }, 0);

	ExecuteEvent(Handle.GetID(), EWorldCursorEventType::Delegate, EInputEvent::IE_Pressed);

	return Handle;
}

void UWorldCursorComponent::RemoveDelegate(const FWorldCursorDelegateHandle& Handle)
{
	if (!IsValidDelegateHandle(Handle))
		return;

	Handle.GetCursor()->ExecuteEvent(Handle.GetID(), EWorldCursorEventType::Delegate, EInputEvent::IE_Released);

	Handle.GetCursor()->Delegates.RemoveAll([&](const auto& Pair) -> bool { return Pair.Key == Handle.GetID(); });
}

//void UWorldCursorComponent::BeginCapture(const FWorldCursorDelegateHandle& Handle, EWorldCursorEventType Type)
//{
//	if (!IsValidDelegateHandle(Handle))
//		return;
//
//	Handle.GetCursor()->BeginCaptureInternal(Handle.GetID(), Type);
//}
//
//void UWorldCursorComponent::BeginCaptureAll(const FWorldCursorDelegateHandle& Handle)
//{
//	if (!IsValidDelegateHandle(Handle))
//		return;
//
//	Handle.GetCursor()->BeginCaptureAllInternal(Handle.GetID());
//}
//
//void UWorldCursorComponent::EndCapture(const FWorldCursorDelegateHandle& Handle, EWorldCursorEventType Type)
//{
//	if (!IsValidDelegateHandle(Handle))
//		return;
//
//	Handle.GetCursor()->EndCaptureInternal(Handle.GetID(), Type);
//}
//
//void UWorldCursorComponent::EndCaptureAll(const FWorldCursorDelegateHandle& Handle)
//{
//	if (!IsValidDelegateHandle(Handle))
//		return;
//
//	Handle.GetCursor()->EndCaptureAllInternal(Handle.GetID());
//}

void UWorldCursorComponent::SelectionPressed()
{
	ExecuteEventStack(EWorldCursorEventType::Selection, EInputEvent::IE_Pressed);
}

void UWorldCursorComponent::SelectionReleased()
{
	ExecuteEventStack(EWorldCursorEventType::Selection, EInputEvent::IE_Released);
}

void UWorldCursorComponent::ContextActionPressed()
{
	ExecuteEventStack(EWorldCursorEventType::ContextAction, EInputEvent::IE_Pressed);
}

void UWorldCursorComponent::ContextActionReleased()
{
	ExecuteEventStack(EWorldCursorEventType::ContextAction, EInputEvent::IE_Released);
}

void UWorldCursorComponent::CursorLocationUpdated(const FWorldCursorLocation& NewCursorLocation)
{
	CursorLocation = NewCursorLocation;

	ExecuteEventStack(EWorldCursorEventType::Location, EInputEvent::IE_Pressed);

	//Route to the one delegate that may have captured the previous press event
	ExecuteEventStack(EWorldCursorEventType::Location, EInputEvent::IE_Axis);
}

void UWorldCursorComponent::BeginCaptureInternal(int32 ID, EWorldCursorEventType Type)
{
	if (Type == EWorldCursorEventType::Delegate)
		//Delegate begin/end is never captured
		return;

	auto& Capture = GetCapture(Type);

	//Clear existing capture if any
	if (Capture && *Capture != ID)
		EndCaptureInternal(*Capture, Type);
	
	Capture = ID;
}

void UWorldCursorComponent::EndCaptureInternal(int32 ID, EWorldCursorEventType Type)
{
	if (Type == EWorldCursorEventType::Delegate)
		return;

	auto& Capture = GetCapture(Type);

	//Only clear capture if this was the delegate that had it
	if (!Capture || *Capture != ID)
		return;

	ExecuteEvent(ID, Type, EInputEvent::IE_Released);
}

//bool UWorldCursorComponent::HasDelegate(int32 ID) const
//{
//	for (auto& [DelegateID, Delegate] : Delegates)
//		if (DelegateID == ID)
//			return true;
//
//	return false;
//}

//void UWorldCursorComponent::BeginCaptureAllInternal(int32 ID)
//{
//	for (int32 i = 0; i < (int32)EWorldCursorEventType::MAX; ++i)
//	{
//		if (Captures[i] && Captures[i] != ID)
//			ExecuteEvent(*Captures[i], (EWorldCursorEventType)i, EInputEvent::IE_Released);
//
//		Captures[i] = ID;
//	}
//
//}
//
//void UWorldCursorComponent::EndCaptureAllInternal(int32 ID)
//{
//	for (int32 i = 0; i < (int32)EWorldCursorEventType::MAX; ++i)
//		if (Captures[i] && *Captures[i] == ID)
//			ExecuteEvent(ID, (EWorldCursorEventType)i, EInputEvent::IE_Released);
//}

EWorldCursorEventResult UWorldCursorComponent::ExecuteEvent(int32 ID, EWorldCursorEventType Type, EInputEvent Input)
{
	if (Type != EWorldCursorEventType::Delegate)
	{
		auto& Capture = GetCapture(Type);

		if (Input == IE_Released)
		{
			if (!Capture || *Capture != ID)
				//Cannot execute release because we never captured the pressed
				return EWorldCursorEventResult::Ignore;

			Capture.Reset();
		}
	}

	FWorldCursorEventDelegate Delegate;

	for (auto& [PairID, PairDelegate] : Delegates)
		if (PairID == ID)
		{
			Delegate = PairDelegate;
			break;
		}

	if (!Delegate.IsBound())
		return EWorldCursorEventResult::Ignore;

	FWorldCursorEventParams Params;

	Params.Cursor = this;

	Params.Location = CursorLocation;

	Params.Type = Type;

	Params.Input = Input;

	Params.Handle = FWorldCursorDelegateHandle{ this, ID };

	return Delegate.Execute(Params);
}

void UWorldCursorComponent::ExecuteEventStack(EWorldCursorEventType Type, EInputEvent Input)
{

	if (Input == IE_Released || Input == IE_Axis)
	{
		//End or Axis event, can only ever route to the single delegate that has previously captured the begin input
		if (auto Capture = GetCapture(Type))
			ExecuteEvent(*Capture, Type, Input);

		return;
	}
	else if (Input == IE_Pressed)
	{
		//Visit the current highest priority delegate until we've visited everything on the stack

		TSet<int32> VisitedDelegates;

		VisitedDelegates.Reserve(Delegates.Num());

		TSet<int32> CurrentDelegates;

		for (bool bVisitedDelegate = true; bVisitedDelegate;)
		{
			bVisitedDelegate = false;

			CurrentDelegates.Empty(Delegates.Num());

			for (auto& [ID, Delegate] : Delegates)
				CurrentDelegates.Add(ID);

			for (int32 i = 0; i < Delegates.Num(); ++i)
			{
				auto ID = Delegates[i].Key;

				if (VisitedDelegates.Contains(ID))
					continue;

				bVisitedDelegate = true;

				VisitedDelegates.Add(ID);

				if (Type == EWorldCursorEventType::Location)
					if (auto Capture = GetCapture(EWorldCursorEventType::Location))
						if (*Capture == ID)
							//Don't let anything below preempt the existing location capture
							return;

				auto Result = ExecuteEvent(ID, Type, Input);

	
				if (Result == EWorldCursorEventResult::Ignore)
				{
					continue;
				}
				else if (Result == EWorldCursorEventResult::Handled)
				{
					//Don't let anything originally on the stack receive the event, we only allow new delegates to receive it
					VisitedDelegates.Append(CurrentDelegates);
					break;
				}
				else if (Result == EWorldCursorEventResult::Captured)
				{
					BeginCaptureInternal(ID, Type);
					return;
				}

			}
		}

	}

}

FWorldCursorDelegateHandle UWorldCursorComponent::NewDelegateHandle()
{
	return FWorldCursorDelegateHandle{ this, NextID++ };
}

