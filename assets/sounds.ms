// Demo of loaded and dynamically-created sounds

// Loading from a WAV file: easy!
wooble = raylib.LoadSound("assets/wooble.wav")

// Creating a sound dynamically: a little more work!
// We use the CreateWave (non-standard extension method) to create
// a sound from either a list of numbers, or a RawData.

// And to make it slightly easier here, we make a helper method that
// assumes these parameters:
sampleRate = 11025  // classic retro sound quality
bitsPerSample = 8
channels = 1
// ...and builds a Sound from just a set of samples (list or RawData).
makeSound = function(samples)
	wave = raylib.CreateWave(samples.len, sampleRate, bitsPerSample, channels, samples)
	sound = raylib.LoadSoundFromWave(wave)
	raylib.UnloadWave wave
	return sound
end function

// boop: a simple square wave
duration = 0.25
frequency = 261.6 // Middle C
sampleCount = sampleRate * duration
samples = [0] * sampleCount
samplesPerCycle = sampleRate / frequency
i = 0
while i < samples.len
	posInCycle = i % samplesPerCycle
	samples[i] = 64 + 128 * (posInCycle < samplesPerCycle / 2)
	i += 1
end while
boop = makeSound(samples)

// pew: a laser sound (frequency starts high, but quickly falls)
duration = 0.5
samplesPerCycle = 1
i = 0
dAmp = 127
leftThisCycle = samplesPerCycle
while i < samples.len	
	samples[i] = 128 + dAmp
	i += 1
	leftThisCycle -= 1
	if leftThisCycle <= 0 then
		samplesPerCycle *= 1.01  // increasing wavelength -> decreasing pitch
		leftThisCycle = floor(samplesPerCycle)
		dAmp = -dAmp * 0.99		// decreasing amplitude (fade out over time)
	end if
end while
pew = makeSound(samples)

// white: just 3 seconds of white noise (for engine sounds, etc.)
duration = 3
i = 0
samples = [0] * sampleRate * duration
while i < samples.len	
	samples[i] = 255 * rnd	// random, white noise
	i += 1
end while
white = makeSound(samples)

// hit: a quick white noise sound, like a small impact
duration = 1
i = 0
amp = 127
samples = [0] * sampleRate * duration
while i < samples.len	
	samples[i] = 128 - amp + amp * 2 * rnd	// random, white noise
	i += 1
	amp *= 0.99  // fade out over time
end while
hit = makeSound(samples)

// boom: a longer (slower-fading) white noise sound, good for explosions
duration = 2
i = 0
amp = 127
samples = [0] * sampleRate * duration
while i < samples.len	
	samples[i] = 128 - amp + amp * 2 * rnd	// random, white noise
	i += 1
	amp *= 0.999  // fade out over time
end while
boom = makeSound(samples)



sounds = [null, wooble, boop, pew, white, hit, boom]

run = function
	x = 100; y = 100
	dx = 10; dy = 10
	
	while true
		raylib.BeginDrawing
		raylib.ClearBackground raylib.DARKGRAY
		
		y = 100
		raylib.DrawText "Press a key to hear the sound:", 100, y, 20, raylib.LIME
		y += 40
		for i in range(1, sounds.len-1)
			name = outer.indexOf(sounds[i])
			raylib.DrawText str(i) + ". " + name, 140, y, 20, raylib.LIME
			if raylib.IsKeyPressed(raylib.KEY_ZERO + i) then raylib.PlaySound sounds[i]
			y += 24
		end for
		raylib.EndDrawing
		yield
	end while
end function

if locals == globals then run
