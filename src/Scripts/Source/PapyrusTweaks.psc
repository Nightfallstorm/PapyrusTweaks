Scriptname PapyrusTweaks
{}

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

bool Function IsMainThreadTweakActive() global native

; Disables Main Thread Tweak for just the current event that calls this function. 
; This will only disable the Main Thread Tweak for the calling event, all other scripts will process at fast mode unless they also call this function'
; This does nothing if Main Thread Tweak is disabled
; EXAMPLE:
;
;Event SomeEvent
;    SomeQuest.doAThing() ; Is Fast
;EndEvent

;Event OnActivate(ObjectReference activator) 
;    if (PapyrusTweaks.IsPapyrusTweaksInstalled())
;        PapyrusTweaks.DisableFastMode()
;    endif
;    
;    SomeQuest.doAThing() ; Is Not Fast
;
;    PapyrusTweaks.EnableFastMode()
;   
;    SomeQuest.doAThing() ; Is Fast
;       
;
;EndEvent
bool Function DisableFastMode() global native

; Enables Main Thread Tweak for just the current event that calls this function
; This does nothing if DisableFastMode() was not called in the current event
bool Function EnableFastMode() global native





