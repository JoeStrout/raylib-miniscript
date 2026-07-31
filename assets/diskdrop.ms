// diskdrop.ms
// Demo and manual test for mounting user disks by drag and drop.
//
// Drop a folder on the left half of the window to mount it as /usr, or on the
// right half for /usr2.  Nothing here ever sees a host path: file.droppedFiles
// reports base names only, and file.mountDropped names the file by index.
//
// Routing on *position* rather than a modifier key is not a style choice.  A
// drag comes from another application, so this window has received no key
// events and file.dropPosition is the only context a drop actually carries.
//
// Run:  ./build/raylib-miniscript assets/diskdrop.ms

windowWidth = 800
windowHeight = 480
if not raylib.IsWindowReady then raylib.InitWindow windowWidth, windowHeight
raylib.SetWindowTitle "Disk Drop Test"

// Sandbox mode is what makes /usr mean anything: once latched, only mounted
// disks are reachable.  Comment this out to compare with the unsandboxed case.
file.enterSandbox

status = "Drop a folder on the left for /usr, on the right for /usr2."

listDisks = function
	lines = []
	for name in file.children("/")
		count = file.children("/" + name).len
		lines.push "/" + name + "  (" + count + " item" + ("s" * (count != 1)) + ")"
	end for
	if not lines then lines.push "(no disks mounted)"
	return lines
end function

handleDrop = function(dropped)
	slot = "usr"
	if file.dropPosition.x > windowWidth/2 then slot = "usr2"
	if file.mountDropped(0, slot) then
		globals.status = "Mounted " + dropped[0].name + " as /" + slot
	else if dropped[0].isDirectory then
		globals.status = "Could not mount " + dropped[0].name
	else
		// Until the zip backend lands (step 6), only folders are disks.
		globals.status = dropped[0].name + " is not a folder"
	end if
	file.clearDroppedFiles
end function

while not raylib.WindowShouldClose
	dropped = file.droppedFiles
	if dropped then handleDrop dropped
	if raylib.IsKeyPressed(raylib.KEY_U) then
		status = "Unmounted /usr: " + file.unmount("usr")
	end if
	if raylib.IsKeyPressed(raylib.KEY_I) then
		status = "Unmounted /usr2: " + file.unmount("usr2")
	end if

	raylib.BeginDrawing
	raylib.ClearBackground raylib.RAYWHITE
	raylib.DrawLine windowWidth/2, 90, windowWidth/2, windowHeight-40, raylib.LIGHTGRAY
	raylib.DrawText "drop here for /usr", 60, 100, 16, raylib.LIGHTGRAY
	raylib.DrawText "drop here for /usr2", windowWidth/2 + 60, 100, 16, raylib.LIGHTGRAY

	raylib.DrawText "Disk mounting by drag and drop", 20, 20, 24, raylib.DARKBLUE
	raylib.DrawText status, 20, 60, 18, raylib.MAROON
	raylib.DrawText "U = unmount /usr    I = unmount /usr2", 20, windowHeight-30, 14, raylib.GRAY

	y = 150
	raylib.DrawText "Mounted:", 20, y, 18, raylib.DARKGRAY
	y = y + 28
	for line in listDisks
		raylib.DrawText line, 40, y, 18, raylib.BLACK
		y = y + 24
	end for
	raylib.EndDrawing
end while

raylib.CloseWindow
