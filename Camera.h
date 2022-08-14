#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum Camera_Movement {	// Camera movement enum
	C_FORWARD,
	C_BACKWARD,
	C_LEFT,
	C_RIGHT
};

// Default parameters
const float D_YAW = -90.0f;
const float D_PITCH = 0.0f;
const float D_SPEED = 0.01f;
const float D_SENSITIVITY = 0.1f;
const float D_ZOOM = 60.0f;			// Personal 60 fov preference

class Camera
{
public:

	// Camera attribute vectors
	glm::vec3 Position;
	glm::vec3 Front;
	glm::vec3 Up;
	glm::vec3 Right;
	glm::vec3 WorldUp;

	// Angles
	float Yaw;
	float Pitch;
	// float Roll; --> Not using roll angle atm

	// Camera Params
	float MovementSpeed;
	float MouseSensitivity;
	float Zoom;

	int xChunk;
	int zChunk;

	bool updateWorldData = false;

	// Constructor (vectors)
	Camera( glm::vec3 position = glm::vec3( 0.0f, 0.0f, 0.0f ), glm::vec3 up = glm::vec3( 0.0f, 1.0f, 0.0f ), float yaw = D_YAW, float pitch = D_PITCH ) : Front( glm::vec3( 0.0f, 0.0f, 1.0f ) ), MovementSpeed( D_SPEED ), MouseSensitivity( D_SENSITIVITY ), Zoom( D_ZOOM )
	{
		Position = position;
		WorldUp = up;
		Yaw = yaw;
		Pitch = pitch;
		updateCameraVectors();
	}
	// Constructor (values)
	Camera( float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch ) : Front( glm::vec3( 0.0f, 0.0f, 1.0f ) ), MovementSpeed( D_SPEED ), MouseSensitivity( D_SENSITIVITY ), Zoom( D_ZOOM )
	{
		Position = glm::vec3( posX, posY, posZ );
		WorldUp = glm::vec3( upX, upY, upZ );
		Yaw = yaw;
		Pitch = pitch;
		updateCameraVectors();
	}

	// Return the view matrix of the camera
	glm::mat4 getViewMatrix()
	{
		return glm::lookAt( Position, Position + Front, Up );
	}

	// Process input from keyboard using defined ENUM
	void processKeyboard( Camera_Movement direction, float deltaTime )
	{
		float velocity = MovementSpeed;// *deltaTime;
		if( direction == C_FORWARD )
			Position += Front * velocity;
		if( direction == C_BACKWARD )
			Position -= Front * velocity;
		if( direction == C_LEFT )
			Position -= Right * velocity;
		if( direction == C_RIGHT )
			Position += Right * velocity;

		// No need to divide by 16 since player coords are in 1/16th scale
		int currXChunk = Position.x; 
		int currZChunk = Position.z;

		if ( xChunk != currXChunk ) {
			xChunk = currXChunk;
			updateWorldData = true;
		}
		if ( zChunk != currZChunk ) {
			zChunk = currZChunk;
			updateWorldData = true;
		}

	}

	// Processes X/Y offset into yaw/pitch changes respectively
	void processMouseMovement( float xoffset, float yoffset, GLboolean constrainPitch = true )
	{
		xoffset *= MouseSensitivity;
		yoffset *= MouseSensitivity;

		Yaw += xoffset;
		Pitch += yoffset;

		// Contrain pitch if enabled (stops screen from flipping)
		if( constrainPitch )
		{
			if( Pitch > 89.0f )
				Pitch = 89.0f;
			if( Pitch < -89.0f )
				Pitch = -89.0f;
		}
		// Update internal vectors using new euler angles
		updateCameraVectors();
	}

	// Processess Y scroll into zoom (FOV) changes
	void processMouseScroll( float yoffset )
	{
		Zoom -= ( float )yoffset;
		if( Zoom < 1.0f )
			Zoom = 1.0f;
		if( Zoom > D_ZOOM )	// Max zoom at default zoom value (can be changed to some const MAX_FOV later)
			Zoom = D_ZOOM;
	}

private:
	// Recalculates the internal Front, Up, and C_RIGHT vectors 
	void updateCameraVectors()
	{
		glm::vec3 front;
		front.x = cos( glm::radians( Yaw ) ) * cos( glm::radians( Pitch ) );
		front.y = sin( glm::radians( Pitch ) );
		front.z = sin( glm::radians( Yaw ) ) * cos( glm::radians( Pitch ) );

		Front = glm::normalize( front );
		Right = glm::normalize( glm::cross( Front, WorldUp ) );
		Up = glm::normalize( glm::cross( Right, Front ) );
	}
};

#endif