I want to add some improvement on the UI web tool we developed.

# Improvement on the `SIMULATION` section

i would like to improve the simulation section where i can simulate the screen and the interaction loaded by the json file. 

it is important that all the functionality described in the json are reflected 1:1 in the simulation tool.

i want to be able to see which function woudl have been called when a event happen on the ui.

for instance if pressing a button that would scroll a page i want to see the exact function that would be called.

if i am going to edit a value i want to see that the edit function is called

if i am going to save a value i want to see the function that woudl be called.


in addition i want to be able to change values as it would be done in reality on the device using that json definition.

# Improvement on the `DESIGN` section

i would like to improve the design tool in order to make it a real display editing tool.

i want to be able to:
- insert text
- insert boxes
- insert the  value place holder: this are special places where for instance values coming from the memory are read and displayed and if modified they show the modified values
- add or remove new screen
- add or remove the svg for the animation and its placement: this should act like a box design describe above. This box has a special setting where i can select the single svg frames and obsiously the json that describe the ui must be adapted to have this functionality.
- i want to be able to design a scroll bar where i can define how many steps it has and which step it shoudl display, for instance: i am designing a state with N screen i want to be able to add a scroll bar that i can define having N state and then for each screen on this state it will simply reflect the current screen value we are displaying. (for this functionality if you need clarification please prepare section in the requirements).
- i want to be able to see the list of avaiable functions that can be called when an event is happening
- i want to be able to define which events in a screen are fireable
- i want to be able to assign to each event the function to call when the event 
- i want to be able to also edit "live" json text and when i edit it it must be reflected in the ui immediatelly, if i insert a incorrect or invalid json the ui should alert me immediately (take as an example mermaid that indicate where the code wrong with hints)
- i want to be able to see the transaction effect on the screen


# Improvement on the `Export import` section
i would like to improve this section too.

- i want to be able to import the functions that are avaiable from the firmware definition.

- the json representing the ui must implement all the new features that we described above. 
- the code generator must to take into account the new features that we have impelmented


# Improvement on the `help` section
the help section must be very thorough on describing the different part of the json.
