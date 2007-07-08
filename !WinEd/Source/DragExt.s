;DragExt code source


	GET	h.RegDefs
	GET	h.SWINos
	GET	h.PlotCodes

	AREA	|Drag$$Data|,CODE,READONLY

	EXPORT	Start_Address
	EXPORT	Draw_Offset
	EXPORT	Remove_Offset
	EXPORT	Move_Offset
	EXPORT	Data_Offset
	EXPORT  End_Offset

Start_Address	DCD	Code_Start
Draw_Offset	DCD	Draw_Box - Code_Start
Remove_Offset	DCD	Remove_Box - Code_Start
Move_Offset	DCD	Move_Box - Code_Start
Data_Offset	DCD	Bits_Data - Code_Start
End_Offset	DCD	Code_End - Code_Start

Code_Start

Draw_Box
	STMFD	sp!,{r0-r3,lr}
	;R12 now points to data itself, not pointer to pointer

	BL	Plot_Box

	LDMFD	sp!,{r0-r3,pc}

Remove_Box
	STMFD	sp!,{lr}
	BL	Plot_Box
	LDMFD	sp!,{pc}

Move_Box
	CMP	r0,r4
	CMPEQ	r1,r5
	CMPEQ	r2,r6
	CMPEQ	r3,r7
	;If box hasn't moved, just replot it
	BEQ	Plot_Box
	STMFD	sp!,{lr}
	;Plot over old box
	BL	Plot_Box2
	;Round and plot new box
	BL	Draw_Box
	LDMFD	sp!,{pc}

;Round and plot dotted box with coordinates in r0-r3
Plot_Box
	STMFD	sp!,{r0-r7,lr}
	;Store co-ordinates in higher registers
	MOV	r4,r0
	MOV	r5,r1
	MOV	r6,r2
	MOV	r7,r3
	BL	Plot_Box2
	LDMFD	sp!,{r0-r7,pc}

;Round and plot dotted box with coordinates in r4-r7
Plot_Box2
        STMFD	sp!,{r0-r7,lr}

	;Round min/max x
	LDR	r0,[r12,#8]
	BIC	r4,r4,r0
	BIC	r6,r6,r0
	LDR	r0,[r12]
	ORR	r4,r4,r0
	ORR	r6,r6,r0
	;Round min/max y
	LDR	r0,[r12,#12]
	BIC	r5,r5,r0
	BIC	r7,r7,r0
	LDR	r0,[r12,#4]
	ORR	r5,r5,r0
	ORR	r5,r5,r0

	;Set colour
	;MOV	r0,#3
	;MOV	r1,#0
	;SWI	XOS_SetColour

	;Move to bottom left
	MOV	r0,#Move_Absolute
	MOV	r1,r4
	MOV	r2,r5
	SWI	XOS_Plot
	;Draw to bottom right
	MOV	r0,#22
	MOV	r1,r6
	MOV	r2,r5
	SWI	XOS_Plot
	;Draw to top right
	MOV	r0,#54
	MOV	r1,r6
	MOV	r2,r7
	SWI	XOS_Plot
	;Draw to top left
	MOV	r0,#54
	MOV	r1,r4
	MOV	r2,r7
	SWI	XOS_Plot
	;Draw to bottom left
	MOV	r0,#62
	MOV	r1,r4
	MOV	r2,r5
	SWI	XOS_Plot

	LDMFD	sp!,{r0-r7,pc}

Bits_Data
	DCD	0,0,0,0

Code_End

	END
