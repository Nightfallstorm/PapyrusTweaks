Scriptname PapyrusTest_InvalidFunctionCall extends ObjectReference
{The documentation string.}
import PapyrusTest_InvalidFunctionCallMethods

Actor Property playerRef Auto

Event OnActivate(ObjectReference akActionRef)
    Debug.Messagebox("Initiating Various Invalid function calls...")

    String result = "Results: " + PapyrusFunctionCallTest() + "/X" 
    Debug.Messagebox("Invalid function calls complete")
    ; code
EndEvent

int Function PapyrusFunctionCallTest()
    int result = 0
    result += correctIntCall(int any)
    result += correctObjectCall(Form any)
    result += correctStringCall(String any)
    result += invalidIntCall(int passNonIntHere)
    result += invalidObjectCall(Form passIntHere)
    result += invalidStringCall(String passNonIntHere)
    result += invalidStringCall2(String passObjectHere)
    result += invalidBooleanCall(bool passIntHere)
    result += invalidObjectCall(Quest passNonQuestHere)
    return result
endFunction