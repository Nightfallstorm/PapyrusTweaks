Scriptname PapyrusTweaks
{ Set of functions for manipulating papyrus tweaks for scripts that need it }

;       _                        
;       \`*-.                    
;        )  _`-.                 
;       .  : `. .                
;       : _   '  \               
;       ; *` _.   `*-._          
;       `-.-'          `-.       
;         ;       `       `.     
;         :.       .        \    
;         . \  .   :   .-'   .   
;         '  `+.;  ;  '      :   
;         :  '  |    ;       ;-. 
;         ; '   : :`-:     _.`* ;
;[bug] .*' /  .*' ; .*`- +'  `*' 
;      `*-*   `*-*  `*-*'

 

;Returns current papyrus tweaks version
int[] Function GetPapyrusTweaksVersion() global native

; Deprecated, use IsNativeCallSpeedUpActive() instead
bool Function IsMainThreadTweakActive() global
    return IsNativeCallSpeedUpActive()
endFunction

bool Function IsNativeCallSpeedUpActive() global native

; Disables Native Call Speed Up for just the current event that calls this function. 
; This will only disable the Main Thread Tweak for the calling event, all other scripts will process at fast mode unless they also call this function'
; This does nothing if Main Thread Tweak is disabled
; EXAMPLE:
;
;Event SomeEvent
;    SomeQuest.doAThing() ; Is Fast, because DisableFastMode() was not called for this event, even if DisableFastMode() was called in OnActivate before executing this event
;EndEvent

;Event OnActivate(ObjectReference activator) 
;    if (PapyrusTweaks.IsPapyrusTweaksInstalled())
;        PapyrusTweaks.DisableFastMode()
;    endif
;    
;    SomeQuest.doAThing() ; Is Not Fast (aka normal mode), and every native function called in "doAThing" will also be normal mode
;
;    PapyrusTweaks.EnableFastMode()
;   
;    SomeQuest.doAThing() ; Is Fast, and every native function called in "doAThing" will also be fast
;       
;
;EndEvent
bool Function DisableFastMode() global native

; Re-enables Native Call Speed Up for just the current event that calls this function
; This does nothing if DisableFastMode() was not called in the current event. 
; In other words, this cannot be used to enable native call speed up if the user has it disabled in their INI settings
bool Function EnableFastMode() global native





