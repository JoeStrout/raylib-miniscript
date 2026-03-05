Sprite = {}
Sprite.x = 0
Sprite.y = 320
Sprite.rotation = 0
Sprite.scale = 1
Sprite.srcTexture = null // must be defined
Sprite.srcRect = null    // optional: [left, top, width, height] in srcTexture
Sprite.tint = raylib.WHITE

Sprite.update = null	// define your own update method if needed

Sprite.draw = function
	if self.srcRect then
		srcRect = self.srcRect
	else
		srcRect = [0, 0, self.srcTexture.width, self.srcTexture.Height]
	end if
	destWidth = srcRect[2] * self.scale
	destHeight = srcRect[3] * self.scale
	destRect = [self.x, self.y, destWidth, destHeight]
	origin = [destWidth * 0.5, destHeight * 0.5]
	raylib.DrawTexturePro self.srcTexture, srcRect, destRect,
		origin, self.rotation, self.tint
end function

return Sprite
