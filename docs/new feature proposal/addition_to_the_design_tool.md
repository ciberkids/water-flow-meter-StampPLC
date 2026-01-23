# improvement on the design tool

i want to improve the design tool in order to make it a real display editing tool.

i want to be able to:

## indicate when a "value" should have a update function defined in the firmware

basically the design tool is a bridge between:

- the firmware functions
- the library that draw screens on the display

when i am designing the display interaction and screens i needs basically to be able to tie together events like:

- i press a button and that button has this meaning NOW
- the firmware has an updated value and i want to show it on the display
- the button pressed should now increas the value and the value shoudl be represented on the display
- i press the save button and the value should be saved and i should be able to indicate which screen should be loaded after.

as you can see in the docs the screen has already a described behaviour but clearly with this ui i cannot customize it or change it in the right way.

so we need to add at loading time a manifest where these functions are defined, and hten these functions are available in the design tool.
we need also to have the function for the setting/saving of value

we need there fore 2 types of function:

- function that are called when an event happen
- function that are called to update a value

for the first type we need to have a list of all the function available and the events that can trigger them.
for the second type we need to have a list of all the function available and the values that can trigger them.

the ui need to be able to select the function and the event or value that trigger it and this setting is connected with the type of tool we are designing

basically "value" is a placeholder for a value that can be updated by the firmware and "event" is a placeholder for an event that can be triggered by the user.

when the design is concluded then the code generated will convert the json file that is representing the UI into cpp code.

if something is not clear please ask for clarification.
