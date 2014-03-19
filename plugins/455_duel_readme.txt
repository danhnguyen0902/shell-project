Plugin: duel

Description: 
	Duel is a game agains the computer remeniscent of Rock Paper 
	Scissors. Each player will start with 10 health . Each round 
	continues until one person gets hit. At the end of a round
	the "charges" saved up by the players get reset." If both 
	players attack at the same time, both players will be damaged.
	Choices:
		1: shield
			blocks damage
		2: attack
			attacks the computer
		3: charge
			charges up and is stored for later in the round
	Charging: 
		1 charge: 2 damage
		2 charges: 3 damage
		3 charges: 5 damage and cannot be blocked

	If you give the program and invalid choice for an attack it
	will automatically choose shield.

Implementation:
	duel
		This takes no arguments
