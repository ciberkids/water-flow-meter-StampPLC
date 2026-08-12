Given that is difficult to visualize the behaviour of the display without having the device, i want to have a mean to translate visual to the behaviour of the device i want to craete the following new features:
1) a web application that showcase all the screen and interactions that the requirements describe.
2) the web application is showing a 1:1 size of the display (maybe zoomed on the screen showing the pixel bigger)
3) the web application will allow to simulate the interaction with the ui via the buttons (as they are in the device)
4) the web applicaiton will allow to change or move the items in every screen/template in order to have a easy editing.
5) the look and feel should be simple yet modern, ans easy to render 
6) this application is place on the side of the src code for the firmware and it is written in react + css
7) it has to be possible to change the css in order to change the colors scheme and the anymation type easily so we can create customizations
8) we need a converter engine that translate the react animation and css into tthe cpp code that then can be implemented on the device
8.1) you can use any langue you think is appropriate for this translation engine
8.2) the translation engine should be invokable from the ui via a button/page and the output should be put in the right position in the firmare code, before to overwrite the previous "version of the UI" a back must be made in a folder with the date as name
9) the translation engine MUST have has foundation the code speed, and it has to be very well optimized
10) this require also a refactor of the source code of the firmware with a more folder structure where we can place in the output of the translator easily in order to change the graphic design without disrupting the firmaware itself

when completed we need to verify that all the requirements are still valid