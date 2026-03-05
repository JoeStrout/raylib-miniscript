// "Retro game sound effects", by Vircon32 (Carra). 
// Published at OpenGameArt under license CC-BY 4.0.

rl = raylib
rl.BeginDrawing
rl.ClearBackground rl.BLACK
rl.EndDrawing
yield

import "mathUtil"
import "listUtil"

blockColors = [
	[242, 110, 38, 255],
	[255, 217, 77, 255],
	[196, 54, 236, 255],
	[77, 234, 233, 255],
]

hitBlockSound = rl.LoadSound("assets/sfx/SoundCoin.wav")
hitBounceSound = rl.LoadSound("assets/sfx/SoundBounce.wav")
loseSound = rl.LoadSound("assets/sfx/SoundFallDull.wav")

screenWidth = rl.GetScreenWidth
screenHeight = rl.GetScreenHeight

Point = {}
Point.Create = function(x,y)
	p = {}
	p.x = x
	p.y = y
	return p
end function

Rectangle = {}
Rectangle.Create = function(x,y,w,h)
	r = Point.Create(x,y)
	r.width = w
	r.height = h
	return r
end function

Movable = {}
Movable.Create = function(x,y,w,h,s)
	m = Rectangle.Create(x,y,w,h)
	m.speed = s
	m.prev = Point.Create(x,y)
	m.render = Point.Create(x,y)
	return m
end function

Ball = {}
Ball.Create = function(x,y,w,h,s,dx,dy)
	b = Movable.Create(x,y,w,h,s)
	b.radius = w/2
	b.dir = Point.Create(dx,dy)

	b.handle_collision = function(rect, allowSideCheck=true, leftBlockExists=false, rightBlockExists=false)
		collisionNormal = {"x": 0, "y": 0}
		
		if self.prev.y < rect.y then
			collisionNormal.y = -1
			self.y = rect.y - self.radius
		else if self.prev.y > rect.y + rect.height then
			collisionNormal.y = 1
			self.y = rect.y + rect.height + self.radius
		end if
		
		if allowSideCheck then
			if self.prev.x < rect.x and not leftBlockExists then
				collisionNormal.x = -1
				self.x = rect.x - self.radius
			else if self.prev.x > rect.x + rect.width and not rightBlockExists then
				collisionNormal.x = 1
				self.x = rect.x + rect.width + self.radius
			end if
		else
			if self.prev.x < rect.x then
				collisionNormal.x = -1
			else if self.prev.x > rect.x + rect.width then
				collisionNormal.x = 1
			end if
		end if
		
		if collisionNormal.x != 0 or collisionNormal.y != 0 then
			magnitude = sqrt(collisionNormal.x ^ 2 + collisionNormal.y ^ 2)
			collisionNormal.x /= magnitude
			collisionNormal.y /= magnitude
			self.dir = self.reflect(self.dir, collisionNormal)
		end if
		
		return collisionNormal
	end function

	b.reflect = function(vector, normal)
		dotProd = vector.x * normal.x + vector.y * normal.y
		v = {
			"x": vector.x - 2 * dotProd * normal.x,
			"y": vector.y - 2 * dotProd * normal.y,
		}
		mag = sqrt(v.x^2 + v.y^2)
		v.x /= mag
		v.y /= mag
		return v
	end function

	return b
end function

Block = {}
Block.Create = function(x,y,w,h,c)
	b = Rectangle.Create(x,y,w,h)
	b.color = c
	b.visible = true
	return b
end function

NUM_BLOCKS_X = 10
NUM_BLOCKS_Y = 8
BLOCK_WDITH = 75
BLOCK_HEIGHT = 25


restart = function(paddle, ball, blocks)
	paddle.x = (screenWidth - paddle.width) / 2
	paddle.prev.x = paddle.x
	paddle.render.x = paddle.x
	
	ball.x = screenWidth / 2
	ball.y = screenHeight / 2
	ball.prev.x = ball.x
	ball.prev.y = ball.y
	ball.render.x = ball.x
	ball.render.y = ball.y
	ball.dir.x = 0
	ball.dir.y = 0

	for x in range(NUM_BLOCKS_X - 1)
		for y in range(NUM_BLOCKS_Y - 1)
			blocks[x][y].visible = true
		end for
	end for
	
	return {
		"gameStarted": false,
		"gameWon": false,
		"remainingBlocks": NUM_BLOCKS_X * NUM_BLOCKS_Y,
		"accumulatedTime": 0,
	}
end function

blockExists = function(blocks, x,y)
	if x < 0 or y < 0 or x >= NUM_BLOCKS_X or y >= NUM_BLOCKS_Y then return false
	return blocks[x][y].visible
end function

run = function
	blocks = list.init2d(NUM_BLOCKS_X, NUM_BLOCKS_Y)
	
	for x in range(NUM_BLOCKS_X - 1)
		for y in range(NUM_BLOCKS_Y - 1)
			blocks[x][y] = Block.Create(80 + x * BLOCK_WDITH +x*5, 60 + y * BLOCK_HEIGHT +y*5, BLOCK_WDITH, BLOCK_HEIGHT, blockColors[floor(y/2)])
		end for
	end for

	ball = Ball.Create(0, 160, 32, 32, 260, 0, 0)
	paddle = Movable.Create(480, 550, 200, 24, 800)

	dt = 1 / 60 // Fixed Time Step
	state = restart(paddle, ball, blocks)
	gameStarted = state.gameStarted
	gameWon = state.gameWon
	remainingBlocks = state.remainingBlocks
	accumulatedTime = state.accumulatedTime
	rl.InitAudioDevice
	while true
		if not gameStarted then
			ball.prev.x = ball.x
			ball.prev.y = ball.y
			ball.x = cos(rl.GetTime) * (screenWidth / 2.5) + screenWidth / 2
			ball.y = screenHeight / 2 + 20

			if rl.IsKeyPressed(rl.KEY_SPACE) then
				// Middle of the top of the paddle
				paddle_middle = {
					"x": paddle.x + paddle.width / 2,
					"y": paddle.y,
				}
				// direction from ball to middle of paddle
				dir_x = paddle_middle.x - ball.x
				dir_y = paddle_middle.y - ball.y
				magnitude = sqrt(dir_x * dir_x + dir_y * dir_y)
				ball.dir.x = dir_x / magnitude
				ball.dir.y = dir_y / magnitude
				gameStarted = true
			end if
		else
			accumulatedTime += rl.GetFrameTime
		end if

		while accumulatedTime >= dt
			paddle.prev.x = paddle.x
			paddle.prev.y = paddle.y

			// Move Ball
			ball.prev.x = ball.x
			ball.prev.y = ball.y
			ball.x += ball.dir.x * ball.speed * dt
			ball.y += ball.dir.y * ball.speed * dt

			// Check for collision against edges
			if ball.x + ball.radius > screenWidth then
				ball.x = screenWidth - ball.radius
				ball.dir = ball.reflect(ball.dir, {
					"x": -1,
					"y": 0,
				})
				rl.PlaySound hitBounceSound
			else if ball.x - ball.radius < 0 then
				ball.x = ball.radius
				ball.dir = ball.reflect(ball.dir, {
					"x": 1,
					"y": 0,
				})
				rl.PlaySound hitBounceSound
			else if ball.y - ball.radius < 0 then
				ball.y = ball.radius
				ball.dir = ball.reflect(ball.dir, {
					"x": 0,
					"y": 1,
				})
				rl.PlaySound hitBounceSound
			else if ball.y > screenHeight + ball.radius * 6 then
				rl.PlaySound loseSound
				state = restart(paddle, ball, blocks)
				gameStarted = state.gameStarted
				gameWon = state.gameWon
				remainingBlocks = state.remainingBlocks
				accumulatedTime = state.accumulatedTime
			end if

			// Move Paddle
			velocity = 0
			if rl.IsKeyDown(rl.KEY_LEFT) then velocity -= paddle.speed
			if rl.IsKeyDown(rl.KEY_RIGHT) then velocity += paddle.speed
			paddle.x = mathUtil.clamp(paddle.x + velocity * dt, 0, screenWidth - paddle.width)

			// Ball Collision with paddle
			if rl.CheckCollisionCircleRec(ball, ball.radius, paddle) then
				collisionNormal = ball.handle_collision(paddle, false)
				
				// Add paddle "english" - adjust horizontal direction based on impact position
				if collisionNormal.y != 0 then
					// Calculate relative hit position (-1 to 1, where 0 is center)
					hitPos = (ball.x - (paddle.x + paddle.width / 2)) / (paddle.width / 2)
					hitPos = mathUtil.clamp(hitPos, -1, 1)
					
					// Apply english by adjusting x direction
					ball.dir.x += hitPos * 0.5
					
					// Renormalize direction vector
					mag = sqrt(ball.dir.x^2 + ball.dir.y^2)
					ball.dir.x /= mag
					ball.dir.y /= mag
				end if

				rl.PlaySound hitBounceSound
			end if

			// Collision with Blocks
			hit = false
			for x in range(NUM_BLOCKS_X - 1)
				for y in range(NUM_BLOCKS_Y - 1)
					b = blocks[x][y]
					if not b.visible then continue

					if rl.CheckCollisionCircleRec(ball, ball.radius, b) then
						ball.handle_collision(b, true, blockExists(blocks, x-1, y), blockExists(blocks, x+1, y))
						b.visible = false
						remainingBlocks -= 1
						
						if remainingBlocks == 0 then
							gameWon = true
							gameStarted = false
						end if

						rl.PlaySound hitBlockSound
						
						hit = true
					end if
				end for
				if hit then break
			end for

			accumulatedTime -= dt
		end while

		// Blend Positions on remaining time for Render Position to avoid studder
		blend = accumulatedTime / dt
		ball.render.x = mathUtil.lerp(ball.prev.x, ball.x, blend)
		ball.render.y = mathUtil.lerp(ball.prev.y, ball.y, blend)
		paddle.render.x = mathUtil.lerp(paddle.prev.x, paddle.x, blend)

		rl.BeginDrawing
		rl.ClearBackground rl.BLACK

		rl.DrawRectangleRec [paddle.render.x, paddle.render.y, paddle.width, paddle.height], [38, 78, 146, 255]
		rl.DrawCircleV ball.render, ball.radius, [145, 206, 34, 255]

		for x in range(NUM_BLOCKS_X - 1)
			for y in range(NUM_BLOCKS_Y - 1)
				b = blocks[x][y]
				if b.visible then 
					rl.DrawRectangleRec b, b.color
					rl.DrawRectangleLinesEx b, 4, [64, 64, 64, 128] 
				end if
			end for
		end for

		if not gameStarted and not gameWon then
			rl.DrawText "Press SPACE to Play", 310, 10, 32, rl.WHITE
		else if gameWon then
			rl.DrawText "YOU WIN!", 375, 280, 48, rl.YELLOW
			rl.DrawText "Press SPACE to Play Again", 257, 340, 32, rl.WHITE
			if rl.IsKeyPressed(rl.KEY_SPACE) then
				state = restart(paddle, ball, blocks)
				gameStarted = state.gameStarted
				gameWon = state.gameWon
				remainingBlocks = state.remainingBlocks
				accumulatedTime = state.accumulatedTime
			end if
		end if

		rl.EndDrawing
		yield
	end while
	rl.CloseAudioDevice
end function

if globals==locals then run