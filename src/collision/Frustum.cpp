#include "anki/collision/Frustum.h"
#include "anki/collision/LineSegment.h"
#include "anki/collision/Aabb.h"

namespace anki {

//==============================================================================
// Frustum                                                                     =
//==============================================================================

//==============================================================================
Frustum& Frustum::operator=(const Frustum& b)
{
	ANKI_ASSERT(type == b.type);
	near = b.near;
	far = b.far;
	planes = b.planes;
	trf = b.trf;
	return *this;
}

//==============================================================================
bool Frustum::insideFrustum(const CollisionShape& b) const
{
	for(const Plane& plane : planes)
	{
		if(b.testPlane(plane) < 0.0)
		{
			return false;
		}
	}

	return true;
}

//==============================================================================
// PerspectiveFrustum                                                          =
//==============================================================================

//==============================================================================
PerspectiveFrustum& PerspectiveFrustum::operator=(const PerspectiveFrustum& b)
{
	Frustum::operator=(b);
	fovX = b.fovX;
	fovY = b.fovY;
	eye = b.eye;
	dirs = b.dirs;
	return *this;
}

//==============================================================================
float PerspectiveFrustum::testPlane(const Plane& p) const
{
	float o = 0.0;

	for(const Vec3& dir : dirs)
	{
		LineSegment ls(eye, dir);
		float t = ls.testPlane(p);

		if(t == 0)
		{
			return 0.0;
		}
		else if(t < 0.0)
		{
			o = std::max(o, t);
		}
		else
		{
			o = std::min(o, t);
		}
	}

	return o;
}

//==============================================================================
void PerspectiveFrustum::transformShape()
{
	eye.transform(trf);

	for(Vec3& dir : dirs)
	{
		dir = trf.getRotation() * dir;
	}
}

//==============================================================================
void PerspectiveFrustum::transform(const Transform& trf_)
{
	trf.transform(trf_);
	transformPlanes();
	transformShape();
}

//==============================================================================
void PerspectiveFrustum::getAabb(Aabb& aabb) const
{
	aabb.set(dirs);
	aabb.getMin() += eye;
	aabb.getMax() += eye;
}

//==============================================================================
void PerspectiveFrustum::recalculate()
{
	// Planes
	//
	float c, s; // cos & sine

	Math::sinCos(Math::PI + fovX / 2, s, c);
	// right
	planes[FP_RIGHT] = Plane(Vec3(c, 0.0, s), 0.0);
	// left
	planes[FP_LEFT] = Plane(Vec3(-c, 0.0, s), 0.0);

	Math::sinCos((3 * Math::PI - fovY) * 0.5, s, c);
	// top
	planes[FP_TOP] = Plane(Vec3(0.0, s, c), 0.0);
	// bottom
	planes[FP_BOTTOM] = Plane(Vec3(0.0, -s, c), 0.0);

	// near
	planes[FP_NEAR] = Plane(Vec3(0.0, 0.0, -1.0), near);
	// far
	planes[FP_FAR] = Plane(Vec3(0.0, 0.0, 1.0), -far);

	// Rest
	//
	eye = Vec3(0.0, 0.0, -near);

	float x = far / tan((Math::PI - fovX) / 2.0);
	float y = tan(fovY / 2.0) * far;
	float z = -far;

	dirs[0] = Vec3(x, y, z - near); // top right
	dirs[1] = Vec3(-x, y, z - near); // top left
	dirs[2] = Vec3(-x, -y, z - near); // bottom left
	dirs[3] = Vec3(x, -y, z - near); // bottom right

	// Transform
	//
	transformPlanes();
	transformShape();
}

//==============================================================================
Mat4 PerspectiveFrustum::calculateProjectionMatrix() const
{
	Mat4 projectionMat;

	float f = 1.0 / tan(fovY * 0.5); // f = cot(fovY/2)

	projectionMat(0, 0) = f * fovY / fovX; // = f/aspectRatio;
	projectionMat(0, 1) = 0.0;
	projectionMat(0, 2) = 0.0;
	projectionMat(0, 3) = 0.0;
	projectionMat(1, 0) = 0.0;
	projectionMat(1, 1) = f;
	projectionMat(1, 2) = 0.0;
	projectionMat(1, 3) = 0.0;
	projectionMat(2, 0) = 0.0;
	projectionMat(2, 1) = 0.0;
	projectionMat(2, 2) = (far + near) / (near - far);
	projectionMat(2, 3) = (2.0 * far * near) / (near - far);
	projectionMat(3, 0) = 0.0;
	projectionMat(3, 1) = 0.0;
	projectionMat(3, 2) = -1.0;
	projectionMat(3, 3) = 0.0;

	return projectionMat;
}

//==============================================================================
// OrthographicFrustum                                                         =
//==============================================================================

//==============================================================================
OrthographicFrustum& OrthographicFrustum::operator=(
	const OrthographicFrustum& b)
{
	Frustum::operator=(b);
	left = b.left;
	right = b.right;
	top = b.top;
	bottom = b.bottom;
	obb = b.obb;
	return *this;
}

//==============================================================================
void OrthographicFrustum::transform(const Transform& trf_)
{
	trf.transform(trf_);
	transformPlanes();
	transformShape();
}

//==============================================================================
Mat4 OrthographicFrustum::calculateProjectionMatrix() const
{
	float difx = right - left;
	float dify = top - bottom;
	float difz = far - near;
	float tx = -(right + left) / difx;
	float ty = -(top + bottom) / dify;
	float tz = -(far + near) / difz;
	Mat4 m;

	m(0, 0) = 2.0 / difx;
	m(0, 1) = 0.0;
	m(0, 2) = 0.0;
	m(0, 3) = tx;
	m(1, 0) = 0.0;
	m(1, 1) = 2.0 / dify;
	m(1, 2) = 0.0;
	m(1, 3) = ty;
	m(2, 0) = 0.0;
	m(2, 1) = 0.0;
	m(2, 2) = -2.0 / difz;
	m(2, 3) = tz;
	m(3, 0) = 0.0;
	m(3, 1) = 0.0;
	m(3, 2) = 0.0;
	m(3, 3) = 1.0;

	return m;
}

//==============================================================================
void OrthographicFrustum::recalculate()
{
	// Planes
	planes[FP_LEFT] = Plane(Vec3(1.0, 0.0, 0.0), left);
	planes[FP_RIGHT] = Plane(Vec3(-1.0, 0.0, 0.0), -right);
	planes[FP_NEAR] = Plane(Vec3(0.0, 0.0, -1.0), near);
	planes[FP_FAR] = Plane(Vec3(0.0, 0.0, 1.0), -far);
	planes[FP_TOP] = Plane(Vec3(0.0, -1.0, 0.0), -top);
	planes[FP_BOTTOM] = Plane(Vec3(0.0, 1.0, 0.0), bottom);

	// OBB
	Vec3 c((right + left) * 0.5, (top + bottom) * 0.5, - (far + near) * 0.5);
	Vec3 e = Vec3(right, top, -far) - c;
	obb = Obb(c, Mat3::getIdentity(), e);

	// Transform
	transformPlanes();
	transformShape();
}

} // end namespace