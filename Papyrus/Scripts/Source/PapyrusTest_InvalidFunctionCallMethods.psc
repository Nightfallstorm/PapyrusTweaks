Scriptname PapyrusTest_InvalidFunctionCallMethods extends ObjectReference
{All functions are compiled at ints initially, DO NOT COMPILE THIS }

int function correctIntCall(int any)
    return 1
endFunction

int function correctObjectCall(Form any)
    return 2
endFunction

int function correctStringCall(String any)
    return 4
endFunction

int function invalidIntCall(int passNonIntHere)
    return 8
endFunction

int function invalidObjectCall(Form passIntHere)
    return 16
endFunction

int function invalidStringCall(String passNonIntHere)
    return 32
endFunction

int function invalidStringCall2(String passObjectHere)
    return 64
endFunction

int function invalidBooleanCall(bool passIntHere)
    return 128
endFunction

int function invalidObjectCall(Quest passNonQuestHere)
    return 256
endFunction