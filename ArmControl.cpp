#include "ArmControl.h"

#include <sstream>

ArmControl::ArmControl(std::string host)
{
    client = std::make_unique<WebSocketClient>(host); 

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) { 
        std::cerr << "WSAStartup failed!" << std::endl;
        return;
    }

    client->run(); 
}
 
void ArmControl::updateAnglesOnMsg(std::shared_ptr<GameObject::Map> gameObjects)
{
	std::string msg = client->getMsgFromQueue(); 

	if (msg.size()) {

        std::istringstream ss(msg);

        float angle1, angle2, angle3, angle4;
        char comma; 

        if (ss >> angle1 >> comma >> angle2 >> comma >> angle3 >> comma >> angle4 && comma == ',')
        {
            setNewPos(gameObjects, angle1, angle2, angle3 , angle4);
        }
        else {
            std::cout << "Invalid message format!" << std::endl;
        }
	}
}

void ArmControl::sendMousePosition(glm::vec2 mousePos, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, bool spacePressed)
{
    if (abs(mousePos.x) > 1 || abs(mousePos.y) > 1) {  // if the mouse is outside the window, we dont send the position but we need to keep websocket connection alive so send random msg
        client->sendMessage("r");
        return;
    }
    
    glm::vec3 mouseWorldPos = getMouseProjectionOnPlane(mousePos, viewMatrix, projectionMatrix); 
    glm::vec3 posFromBase = mouseWorldPos - basePos; // offset the base height

    std::string pos = std::to_string(posFromBase.x) + "," + std::to_string(std::max(0.f, ( - posFromBase.y - 0.55f))) + "," + std::to_string(posFromBase.z) + "," + std::to_string(spacePressed);
    client->sendMessage(pos); 
}

/// <summary>
/// to go from 2d to 3 mouse position, we project the position of the mouse to a plan parrallele to the camera  and passing by the base of the robot
/// </summary>
/// <param name="mousePos"></param>
/// <param name="viewMatrix"></param>
/// <param name="projectionMatrix"></param>
/// <returns></returns>
glm::vec3 ArmControl::getMouseProjectionOnPlane(glm::vec2 mousePos, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix)
{
    glm::vec4 nearPoint = glm::vec4(mousePos.x, -mousePos.y, -1.0f, 1.0f);
    glm::vec4 farPoint = glm::vec4(mousePos.x, -mousePos.y, 1.0f, 1.0f);

    // Unproject points from clip space to world space
    glm::mat4 inverseVP = glm::inverse(projectionMatrix * viewMatrix);
    glm::vec4 nearWorld = inverseVP * nearPoint;
    glm::vec4 farWorld = inverseVP * farPoint;

    nearWorld /= nearWorld.w;
    farWorld /= farWorld.w;

    glm::vec3 rayOrigin = glm::vec3(nearWorld);
    glm::vec3 rayDirection = glm::normalize(glm::vec3(farWorld) - rayOrigin);

    glm::vec3 planeNormal = glm::vec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]);  // Camera forward direction

    // Compute intersection using: t = (planeOrigin - rayOrigin) ⋅ planeNormal / (rayDirection ⋅ planeNormal)
    float denom = glm::dot(rayDirection, planeNormal);

    if (glm::abs(denom) > 1e-6) {  // Avoid division by zero
        float t = glm::dot(basePos - rayOrigin, planeNormal) / denom; 
        return rayOrigin + t * rayDirection;  // Intersection point
    }
      
    return glm::vec3(0.0f); 
}

void ArmControl::setNewPos(std::shared_ptr<GameObject::Map> gameObjects, float angle1, float angle2, float angle3, float angle4)
{


    // generate displacement matrix for each links 
    glm::mat4 m1 =      mat4(0                             , angle1, 0, 0, -0.25, 0);
    glm::mat4 m2 = m1 * mat4(-angle2 - glm::pi<float>() / 2, 0     , 0, 0, 1.8  , 0);
    glm::mat4 m3 = m2 * mat4(angle3                        , 0     , 0, 0, 1.7  , 0);

    // calculate the bas position of each links, 
    glm::vec4 posLink1 = m1 * glm::vec4(0, 0, 0, 1);
    glm::vec4 posLink2 = m2 * glm::vec4(0, 0, 0, 1); 
    glm::vec4 posLink3 = m3 * glm::vec4(0, 0, 0, 1);

    // offset to the base position of the arm
    gameObjects->at(links[0]).transform.translation = basePos;
    gameObjects->at(links[1]).transform.translation = glm::vec3(posLink1) + basePos;
    gameObjects->at(links[2]).transform.translation = glm::vec3(posLink2) + basePos;
    gameObjects->at(links[3]).transform.translation = glm::vec3(posLink3) + basePos;
    
    // calculate the angle of each link depending on it angle and the previous ones
    gameObjects->at(links[0]).transform.rotation = { glm::pi<float>()                                 , angle1, 0 };
    gameObjects->at(links[1]).transform.rotation = { -angle2                                          , angle1, glm::pi<float>() / 2 };
    gameObjects->at(links[2]).transform.rotation = { -angle2 + angle3                                 , angle1, glm::pi<float>() / 2 };
    gameObjects->at(links[3]).transform.rotation = { -angle2 + angle3 + angle4 + glm::pi<float>() /2  , angle1, 0 };
}

glm::mat4 ArmControl::mat4(float x, float y, float z, float tx, float ty, float tz)
{
    
    const float c3 = glm::cos(z);
    const float s3 = glm::sin(z);
    const float c2 = glm::cos(x);
    const float s2 = glm::sin(x);
    const float c1 = glm::cos(y);
    const float s1 = glm::sin(y);
    glm::mat4 m = glm::mat4{
        {
            (c1 * c3 + s1 * s2 * s3),
            (c2 * s3),
            (c1 * s2 * s3 - c3 * s1),
            0.0f,
        },
        {
            (c3 * s1 * s2 - c1 * s3),
            (c2 * c3),
            (c1 * c3 * s2 + s1 * s3),
            0.0f,
        },
        {
            (c2 * s1),
            (-s2),
            (c1 * c2),
            0.0f,
        },
        {0.f, 0.f, 0.f , 1.0f} };

    glm::mat4 t = glm::mat4{
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {tx, ty, tz, 1}
    };

    return m * t;
}
