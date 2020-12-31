# Online Games Networks - Spaceship battle, free-for-all!
by Rafel Brau & Òscar Faura
Spaceship battle, free-for-all! is a 2D shooter multiplayer game developed for the subject Online Games & Networks in CITM - UPC Barcelona 
## Gameplay Tutorial & Instructions
Join the server and start shooting the other spaceships. Destroy them to win points and avoid their lasers to be the last one alive!

- **Down Arrow** - Move forward 

- **Left Arrow** - Shoot laser

- **A** - Rotate left 

- **D** - Rotate right

## Implemented features
### Implemented by Òscar
- ** Handle players join/leave events:** Achieved with some bugs. First time a player joins the server there is no problem at all. Also, if the player dies or they press the disconnect button it works perfectly. But, if the player tries to join the server again, once he/she has being disconnected, the game sometimes crashes or the other players can see the spaceship that has just joined, but the player that has connected can not see what is happening until a few seconds later. No idea why it happens.
- ** World State Replication:** Successfully implemented, all players see everything that happens correctly. At first, we had a bug that made the game crash every time a spaceship died. The solution was to insert the 'Destroy' action in the replication commands vector at the beginning.
- ** Delivery Manager:** Successfully implemented. All failed replication deliveries are resend once the delivery manager notices a time out. If a replication delivery received properly, the deliveries are destroy.   
### Implemented by Rafel
- ** Client Side Prediction:** Successfully implemented. If a users has lag and client side prediction is active
- ** Redundancy:** Successfully implemented. It allows to have a stable gameplay when packets are lost.
- ** Kill counter:** When a player kills another one, their kill counter is incremented by one.
