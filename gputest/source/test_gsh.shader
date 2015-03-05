; This is a geometry Shader

; Constants
.constf myconst(0.0, 1.0, 2.0, 3.0)
.alias zeros myconst.xxxx
.alias ones myconst.yyyy

; Outputs
.out outpos position
.out outtc0 texcoord0
.out outtc1 texcoord1
.out outtc2 texcoord2
.out outclr color

.proc main
	; === EMIT THE FIRST TRIANGLE - passed through as-is ===

	; Emit the first vertex of the triangle
	setemit 0
	mov outpos, v0
	mov outtc0, v1
	mov outtc1, v2
	mov outtc2, v3
	mov outclr, v4
	emit

	; Emit the second vertex of the triangle
	setemit 1
	mov outpos, v5
	mov outtc0, v6
	mov outtc1, v7
	mov outtc2, v8
	mov outclr, v9
	emit

	; Emit the third and final vertex of the triangle
	setemit 2, prim
	mov outpos, v10
	mov outtc0, v11
	mov outtc1, v12
	mov outtc2, v13
	mov outclr, v14
	emit

	; === EMIT THE SECOND TRIANGLE - translated and its color inverted ===

	; Adjust the coordinates of the first vertex & invert its color
	add r0, -myconst.xzxx, v0
	mul r0, myconst.zzyy, r0
	add r4.xyz, ones, -v4
	mov r4.w, v4

	; Adjust the coordinates of the second vertex & invert its color
	add r5, -myconst.xzxx, v5
	mul r5, myconst.zzyy, r5
	add r9.xyz, ones, -v9
	mov r9.w, v9

	; Adjust the coordinates of the third vertex & invert its color
	add r10, -myconst.xzxx, v10
	mul r10, myconst.zzyy, r10
	add r14.xyz, ones, -v14
	mov r14.w, v14

	; Emit the first vertex of the triangle
	setemit 0
	mov outpos, r0
	mov outtc0, v1
	mov outtc1, v2
	mov outtc2, v3
	mov outclr, r4
	emit

	; Emit the second vertex of the triangle
	setemit 1
	mov outpos, r5
	mov outtc0, v6
	mov outtc1, v7
	mov outtc2, v8
	mov outclr, r9
	emit

	; Emit the third and final vertex of the triangle
	setemit 2, prim
	mov outpos, r10
	mov outtc0, v11
	mov outtc1, v12
	mov outtc2, v13
	mov outclr, r14
	emit

	end
.end
