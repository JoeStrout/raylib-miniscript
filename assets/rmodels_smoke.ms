// rmodels_smoke.ms
// Quick runtime smoke test for RModels bindings.
//
// Exercises:
// 1) GenMeshCube
// 2) LoadModelFromMesh
// 3) BeginMode3D/EndMode3D + camera helpers
// 4) DrawModel / DrawBoundingBox + 3D primitive draws
// 5) Collision APIs (sphere/box/ray)

rl = raylib

v3 = function(x=0, y=0, z=0)
	return {"x": x, "y": y, "z": z}
end function

boolText = function(b)
	if b then return "true"
	return "false"
end function

run = function
	rl.SetWindowTitle "RModels Smoke"
	rl.SetTargetFPS 60

	mesh = rl.GenMeshCube(2, 2, 2)
	if mesh == null then
		print "GenMeshCube failed"
		return
	end if

	model = rl.LoadModelFromMesh(mesh)
	if model == null then
		print "LoadModelFromMesh failed"
		return
	end if

	modelPos = v3(0, 0, 0)
	modelBounds = rl.GetModelBoundingBox(model)
	cameraPos = v3(6, 6, 6)
	cameraTarget = v3(0, 0, 0)
	cameraUp = v3(0, 1, 0)
	camera = {}
	camera.position = cameraPos
	camera.target = cameraTarget
	camera.up = cameraUp
	camera.fovy = 45
	camera.projection = rl.CAMERA_PERSPECTIVE

	staticSphere = v3(0, 0, 0)
	ray = {}
	ray.position = v3(-5, 0.6, 0)
	ray.direction = v3(1, 0, 0)

	print "RModels smoke script started. Press ESC to stop."
	t = 0
	lastLoggedSecond = -1

	while true
		if rl.IsKeyPressed(rl.KEY_ESCAPE) then break

		dt = rl.GetFrameTime
		t += dt

		movingX = sin(t) * 2.5
		movingSphere = v3(movingX, 0, 0)
		movingBox = {}
		movingBox.min = v3(movingX - 0.75, -1, -1)
		movingBox.max = v3(movingX + 0.75, 1, 1)
		ray.position = v3(-5, 0.6 + sin(t * 1.7) * 0.6, 0)

		rl.UpdateCamera(camera, rl.CAMERA_ORBITAL)
		rl.UpdateCameraPro(camera, v3(0, 0, 0), v3(0, 0, 0), 0)

		screenCenter = [rl.GetScreenWidth() * 0.5, rl.GetScreenHeight() * 0.5]
		aimRay = rl.GetScreenToWorldRay(screenCenter, camera)
		aimRayEx = rl.GetScreenToWorldRayEx(screenCenter, camera, rl.GetScreenWidth(), rl.GetScreenHeight())

		modelScreen = rl.GetWorldToScreen(modelPos, camera)
		modelScreenEx = rl.GetWorldToScreenEx(modelPos, camera, rl.GetScreenWidth(), rl.GetScreenHeight())
		cameraMatrix = rl.GetCameraMatrix(camera)

		hitSpheres = rl.CheckCollisionSpheres(movingSphere, 1.2, staticSphere, 1.0)
		hitBoxes = rl.CheckCollisionBoxes(modelBounds, movingBox)
		hitBoxSphere = rl.CheckCollisionBoxSphere(modelBounds, movingSphere, 0.8)
		hitRayBox = rl.GetRayCollisionBox(ray, modelBounds)
		hitRayMesh = rl.GetRayCollisionMesh(ray, mesh, cameraMatrix)
		hitAimRayBox = rl.GetRayCollisionBox(aimRay, modelBounds)
		hitAimRayExBox = rl.GetRayCollisionBox(aimRayEx, modelBounds)

		thisSecond = floor(t)
		if thisSecond != lastLoggedSecond then
			lastLoggedSecond = thisSecond
			print "[rmodels] spheres=" + boolText(hitSpheres)
			print "[rmodels] boxes=" + boolText(hitBoxes)
			print "[rmodels] boxSphere=" + boolText(hitBoxSphere)
			print "[rmodels] rayBox.hit=" + boolText(hitRayBox.hit) + " dist=" + hitRayBox.distance
			print "[rmodels] rayMesh.hit=" + boolText(hitRayMesh.hit) + " dist=" + hitRayMesh.distance
			print "[rmodels] aimRay.hit=" + boolText(hitAimRayBox.hit) + " aimRayEx.hit=" + boolText(hitAimRayExBox.hit)
			print "[rmodels] camera.m0=" + cameraMatrix.m0 + " screenX=" + modelScreen.x
		end if

		rl.BeginDrawing
		rl.ClearBackground rl.BLACK
		rl.BeginMode3D(camera)

		rl.DrawGrid(20, 1)
		rl.DrawLine3D(staticSphere, movingSphere, rl.YELLOW)
		rl.DrawSphereWires(movingSphere, 0.8, 8, 8, rl.ORANGE)
		rl.DrawRay(aimRay, rl.MAGENTA)
		rl.DrawModel(model, modelPos, 1, rl.WHITE)
		if hitBoxSphere then
			rl.DrawBoundingBox(modelBounds, rl.RED)
		else
			rl.DrawBoundingBox(modelBounds, rl.GREEN)
		end if

		rl.EndMode3D

		rl.DrawText("RModels smoke test", 20, 20, 24, rl.YELLOW)
		rl.DrawText("ESC: exit", 20, 50, 18, rl.RAYWHITE)
		rl.DrawText("3D mode + camera APIs active", 20, 70, 16, rl.SKYBLUE)
		rl.DrawText("CheckCollisionSpheres: " + boolText(hitSpheres), 20, 82, 16, rl.RAYWHITE)
		rl.DrawText("CheckCollisionBoxes: " + boolText(hitBoxes), 20, 104, 16, rl.RAYWHITE)
		rl.DrawText("CheckCollisionBoxSphere: " + boolText(hitBoxSphere), 20, 126, 16, rl.RAYWHITE)
		rl.DrawText("GetRayCollisionBox.hit: " + boolText(hitRayBox.hit), 20, 148, 16, rl.RAYWHITE)
		rl.DrawText("GetRayCollisionMesh.hit: " + boolText(hitRayMesh.hit), 20, 170, 16, rl.RAYWHITE)
		rl.DrawText("CenterRay.hit: " + boolText(hitAimRayBox.hit), 20, 192, 16, rl.RAYWHITE)
		rl.DrawText("CenterRayEx.hit: " + boolText(hitAimRayExBox.hit), 20, 214, 16, rl.RAYWHITE)
		rl.DrawText("Model screen: " + floor(modelScreen.x) + "," + floor(modelScreen.y), 20, 236, 16, rl.RAYWHITE)
		rl.DrawText("Model screenEx: " + floor(modelScreenEx.x) + "," + floor(modelScreenEx.y), 20, 258, 16, rl.RAYWHITE)
		rl.DrawFPS

		rl.EndDrawing
		yield
	end while

	rl.UnloadModel(model)
	print "RModels smoke script finished"
end function

if locals == globals then run
