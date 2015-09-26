# MD5SkeletalAnimation
Skeletal Animation implement with md5 file and Direct3D.骨骼动画核心算法和实现
目标：真正理解骨骼动画原理和核心算法
环境：VS2010及以上版本，Direct3D SDK,编译有问题的请联系我：Jeromecen@hotmail.com qq:1021900404
本项目借鉴了很多网上资料，其中主要是借鉴了http://cspage.net/MD5Loader/ 的OPENGL项目，要OPENGL版本的请到该网址下载资源，非常感谢原作者；还借鉴了http://www.3dgep.com/loading-and-animating-md5-models-with-opengl/ ，和OGRE的动画模块。

实践骨骼动画总结:
1.变换在物体坐标系和坐标体系中的理解：物体连续变换和坐标系变换差异，不同坐标系叉乘和四元数旋转不一样，观察相对位置不同。
骨骼父子节点关系中四元数旋转变换理解，左手坐标系如果不变换标准乘法那么需要(ba) v(ba)-1从后到先来旋转，
右手坐标系在标准乘法下顺时针变则需要(ab)-1v(ab)连续变换即可。
世界坐标系中变换骨骼矩阵整体表达：Mskin-world * (Mbone-world ^ -1) * Mcurrentbone-world的顺序来变换皮肤网格顶点。

2.父子节点变换传递性：父子骨骼节点中的整体变换，可以分解为缩放旋转平移，只有缩放旋转可传递（累加变换），平移不传递（不累加变换，只加结果）。
无论变换子节点还是绑定在节点中的网格顶点变换顺序都是先缩放在旋转，最后平移。

3.蒙皮权重和相对位置：顶点附加到骨骼节点之前还有一个权重，需要累加。在OGRE中动画集合（状态）下面每个动画有多个骨骼节点轨迹，每个节点轨迹有多个keyFrame,
一个动画的一瞬间就有多个动画节点轨迹（其实是一个frame)确定整体骨骼节点位置。OGRE中的蒙皮附加到骨骼节点，骨骼节点的变换是
相对于根骨骼节点的也就是相对于Local Model Space的，没有用相对于世界坐标系，目的是为了插入中间变换层，方便多个实体对象共用
一套相同的骨骼变换。

4.四元数新的理解：
虽然四元数的逆，无论是轴取反还是旋转取反，结果都是（w,-v),但是在变换向量时候，(w,-v)和（-w,v)等价的，证明见：
 (-w1,v1) x (-w1,-v1) = (w1*w1 + v1*v1, w1*v1 -w1*v1 - v1xv1);
 (w1,-v1) x  (w1,v1) = (w1*w1 + v1*v1, w1v1 - w1v1 -v1xv1)
 故在.md5mesh和.md5anim存放的四元数，为了配合骨骼节点右手坐标系中顺时针连续变换(ab)-1v(ab)，则四元数取反为：
 Quaternion MD5Model::buildQuat(float x, float y, float z) const {
	float w = 1.0f - x*x - y*y - z*z;
	w = w < 0.0 ? 0.0f : (float)-sqrt(double(w));
	Quaternion q(x, y, z, w);// 等价于w= sqrt(w)时候的q(-x, -y, -z, w)
	q.normalize();

	return q;
 }
