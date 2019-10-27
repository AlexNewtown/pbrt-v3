
/*
    pbrt source code is Copyright(c) 1998-2016
                        Matt Pharr, Greg Humphreys, and Wenzel Jakob.

    This file is part of pbrt.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:

    - Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    - Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
    IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
    TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
    PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */


// accelerators/kdtreeaccel.cpp*
#include "accelerators/kdtreeaccel.h"
#include "paramset.h"
#include "interaction.h"
#include "stats.h"
#include <algorithm>

namespace pbrt {

// KdTreeAccel Local Declarations
// P285 4.4.1 TREE REPRESENTATION
// Our initial implementation used a 16-byte node representation; when we reduced the size to 8 bytes
// we obtained nearly a 20 % speed increase
// KdAccelNode Ϊ���Ż��ڴ���ʣ������˿ɶ���
struct KdAccelNode 
{
    // KdAccelNode Methods
    void InitLeaf(int *primNums, int np, std::vector<int> *primitiveIndices);
    void InitInterior(int axis, int ac, Float s)
    { 
        split = s;
        flags = axis;

        // �м�ڵ���Ҫ��¼ KdTreeAccel::nodes[] �������ӽڵ��λ��, ����һ��������֮��, ��һ���� aboveChild ��ָʾ
        aboveChild |= (ac << 2);
    }

    Float SplitPos()  const { return split; } // �������������ϻ��ֵ�λ��
    int nPrimitives() const { return nPrims >> 2; } // nPrims �ĸ���ʮλ��¼��Ҷ�ӽڵ��ཻ��ͼԪ����
    int SplitAxis()   const { return flags & 3; } // flags �������λ��¼�� split axis��0/1/2�������x/y/z �Ữ��
    bool IsLeaf()     const { return (flags & 3) == 3; } // �������λΪ 3 ʱ�� ��������һ��Ҷ�ӽڵ�
    int AboveChild()  const { return aboveChild >> 2; }

    union 
    {
        Float split;                 // Interior

        int onePrimitive;            // Leaf, ���ֻ��һ��ͼԪ��Ҷ�ڵ��ཻ, ��¼���ͼԪ�� KdTreeAccel::primitives �е�����, �� onePrimitive Ϊ0ʱ��ʾû�����κ�ͼԪ�ཻ
        int primitiveIndicesOffset;  // Leaf, ������ KdTreeAccel::primitiveIndices ��������¼��һ��ͼԪ���±�, primitiveIndicesOffset ��¼�� primitiveIndices �е��׸��±�
    };

  private:
    // ����ṹ�ĸ���ʮλ�� nPrims/aboveChild ʹ��, �����λ�� flags ʹ��
    union 
    {
        int flags;       // Both
        int nPrims;      // Leaf
        int aboveChild;  // Interior
    };
};

// ��¼�������ϰ�Χ�еı߽�, �ο� Figure4.15
enum class EdgeType { Start, End };
struct BoundEdge 
{
    // BoundEdge Public Methods
    BoundEdge() {}
    BoundEdge(Float t, int primNum, bool starting) : t(t), primNum(primNum) 
    {
        type = starting ? EdgeType::Start : EdgeType::End;
    }

    Float t;
    int primNum;
    EdgeType type;
};

// KdTreeAccel Method Definitions
KdTreeAccel::KdTreeAccel(std::vector<std::shared_ptr<Primitive>> p,
                         int isectCost, int traversalCost, Float emptyBonus,
                         int maxPrims, int maxDepth)
    : isectCost(isectCost),
      traversalCost(traversalCost),
      maxPrims(maxPrims),
      emptyBonus(emptyBonus),
      primitives(std::move(p)) 
{
    // Build kd-tree for accelerator
    ProfilePhase _(Prof::AccelConstruction);

    nextFreeNode = nAllocedNodes = 0;
    // kdtree ����ʱ��Ҫ��һ�����ĵݹ����
    if (maxDepth <= 0)
        maxDepth = std::round(8 + 1.3f * Log2Int(int64_t(primitives.size())));

    // Compute bounds for kd-tree construction
    // optimize: Ԥ�Ȼ���һ��ͼԪ�İ�Χ��
    std::vector<Bounds3f> primBounds;
    primBounds.reserve(primitives.size());
    for (const std::shared_ptr<Primitive> &prim : primitives) 
    {
        Bounds3f b = prim->WorldBound();
        bounds = Union(bounds, b);
        primBounds.push_back(b);
    }

    // Allocate working memory for kd-tree construction
    // ��¼x/y/z �������ϵİ�Χ�еı߽���Ϣ, �ο� Figure4.15
    // һ��ʼ�Ͱ��������������, ����ֻ��Ҫ����һ��, ����������οռ������
    std::unique_ptr<BoundEdge[]> edges[3];
    for (int i = 0; i < 3; ++i)
        edges[i].reset(new BoundEdge[2 * primitives.size()]);

    // ��¼�������ӽڵ��ཻ��ͼԪ, Ҳ��һ��ʼ�Ͱ��������������ڴ�
    // �ݹ黮��������ʱ, ���ܸ��ǵ�������������(prims1), �������� prims ������һ������Ŀռ�
    std::unique_ptr<int[]> prims0(new int[primitives.size()]);
    std::unique_ptr<int[]> prims1(new int[(maxDepth + 1) * primitives.size()]);

    // Initialize _primNums_ for kd-tree construction
    // primNums ��¼��**��ǰ�ڵ�**�ཻ��ͼԪ������, ��ʼʱ����ͼԪ���͸��ڵ��ཻ
    std::unique_ptr<int[]> primNums(new int[primitives.size()]);
    for (size_t i = 0; i < primitives.size(); ++i) 
        primNums[i] = i;

    // Start recursive construction of kd-tree
    buildTree(
        0, 
        bounds, primBounds, 
        primNums.get(), primitives.size(),
        maxDepth, // �������ݹ����, �ڵݹ���� buildTree ��, �� depth == 0 ʱ, ֹͣ�ݹ�
        edges, prims0.get(), prims1.get());
}

void KdAccelNode::InitLeaf(int *primNums, int np,
                           std::vector<int> *primitiveIndices) {
    flags = 3;
    nPrims |= (np << 2);

    // Store primitive ids for leaf node
    if (np == 0) // �սڵ�
        onePrimitive = 0;
    else if (np == 1) // ֻ��һ��ͼԪ�ཻ
        onePrimitive = primNums[0];
    else {
        primitiveIndicesOffset = primitiveIndices->size();
        for (int i = 0; i < np; ++i) primitiveIndices->push_back(primNums[i]);
    }
}

KdTreeAccel::~KdTreeAccel() { FreeAligned(nodes); }

void KdTreeAccel::buildTree(
    int nodeNum, 
    const Bounds3f &nodeBounds, const std::vector<Bounds3f> &allPrimBounds,
    int *primNums, int nPrimitives, 
    int depth,
    const std::unique_ptr<BoundEdge[]> edges[3],
    int *prims0, int *prims1, int badRefines) 
{
    CHECK_EQ(nodeNum, nextFreeNode);

    // Get next free node from _nodes_ array
    // kdtree �ڽ���ʱ�޷�Ԥ��֪��Ҫ��������ڴ�, ��Ҫ�м䲻��ȥ��������
    if (nextFreeNode == nAllocedNodes) 
    {
        int nNewAllocNodes = std::max(2 * nAllocedNodes, 512); // ÿ�ΰ���������
        KdAccelNode *n = AllocAligned<KdAccelNode>(nNewAllocNodes);

        if (nAllocedNodes > 0) {
            memcpy(n, nodes, nAllocedNodes * sizeof(KdAccelNode));
            FreeAligned(nodes);
        }
        nodes = n;
        nAllocedNodes = nNewAllocNodes;
    }
    ++nextFreeNode;

    // Initialize leaf node if termination criteria met
    // ����ڵ��µ�ͼԪ̫��, ���ߴﵽ���ݹ����, �Ͳ������ָ���
    if (nPrimitives <= maxPrims || depth == 0) 
    {
        // �뵱ǰ�ڵ��ཻ��ͼԪ, ͼԪ����, Ҫд��� KdTreeAccel::primitiveIndices
        nodes[nodeNum].InitLeaf(primNums, nPrimitives, &primitiveIndices);
        return;
    }

    // Initialize interior node and continue recursion

    // Choose split axis position for interior node
    // ��Ȼʹ�� Section 4.3.2 �е� SAH ��������ͬ���ַ�ʽ�Ŀ���
    // ��x/y/z���Ϸֱ�ѡȡһ������λ�ò����㿪��, ѡ������͵��Ǹ������ֿռ�
    int bestAxis = -1, bestOffset = -1; // bestOffset: splitOffset in bestAxis

    Float bestCost = Infinity;
    Float oldCost = isectCost * Float(nPrimitives); // defalutCost, �����л���, ����Ҷ�ڵ�Ŀ���

    Float totalSA = nodeBounds.SurfaceArea();
    Float invTotalSA = 1 / totalSA;

    Vector3f d = nodeBounds.pMax - nodeBounds.pMin;

    // Choose which axis to split along
    int axis = nodeBounds.MaximumExtent();
    int retries = 0; // retrySplit �Ĵ���

retrySplit:

    // Initialize edges for _axis_
    // kdtree ������һ���������ϰ�Χ�еı߽紦���л���, �ο� Figure 4.15
    for (int i = 0; i < nPrimitives; ++i) 
    {
        int pn = primNums[i];
        const Bounds3f &bounds = allPrimBounds[pn];
        edges[axis][2 * i] = BoundEdge(bounds.pMin[axis], pn, true);
        edges[axis][2 * i + 1] = BoundEdge(bounds.pMax[axis], pn, false);
    }

    // Sort _edges_ for _axis_
    // ���Ӹ������ķ�������
    std::sort(&edges[axis][0], &edges[axis][2 * nPrimitives],
              [](const BoundEdge &e0, const BoundEdge &e1) -> bool 
              {
                  if (e0.t == e1.t)
                      return (int)e0.type < (int)e1.type;
                  else
                      return e0.t < e1.t;
              });

    // Compute cost of all splits for _axis_ to find best
    int nBelow = 0, nAbove = nPrimitives;
    for (int i = 0; i < 2 * nPrimitives; ++i) 
    {
        // if(i % 2 == 1)
        if (edges[axis][i].type == EdgeType::End) 
            --nAbove;

        Float edgeT = edges[axis][i].t;
        if (edgeT > nodeBounds.pMin[axis] && edgeT < nodeBounds.pMax[axis]) 
        {
            // Compute cost for split at _i_th edge

            // Compute child surface areas for split at _edgeT_
            int otherAxis0 = (axis + 1) % 3, otherAxis1 = (axis + 2) % 3;

            // Vector3f d = nodeBounds.pMax - nodeBounds.pMin;
            // ע����������Ǳ����, ���������
            Float belowSA = 2 * ( d[otherAxis0] * d[otherAxis1] + (edgeT - nodeBounds.pMin[axis]) * (d[otherAxis0] + d[otherAxis1]) );
            Float aboveSA = 2 * ( d[otherAxis0] * d[otherAxis1] + (nodeBounds.pMax[axis] - edgeT) * (d[otherAxis0] + d[otherAxis1]) );

            Float pBelow = belowSA * invTotalSA;
            Float pAbove = aboveSA * invTotalSA;

            // �����һ���ӽڵ��ǿյ�, �Ϳ�����������ڵ�, ������������
            // ���� eb(bonus) ��������Կ�����һ��"����", ������ cost
            Float eb = (nAbove == 0 || nBelow == 0) ? emptyBonus : 0; // emptyBonus ��Ĭ��ֵ�� 0.5f
            Float cost =
                traversalCost +
                isectCost * (1 - eb) * (pBelow * nBelow + pAbove * nAbove);

            // Update best split if this is lowest cost so far
            if (cost < bestCost) 
            {
                bestCost = cost;
                bestAxis = axis;
                bestOffset = i;
            }
        }

        if (edges[axis][i].type == EdgeType::Start) 
            ++nBelow;
    }
    // ��� CHEKC ��Ӧ��ʼ�� 'int nBelow = 0, nAbove = nPrimitives;'
    CHECK(nBelow == nPrimitives && nAbove == 0);

    // Create leaf if no good splits were found
    // Ϊʲô����� Figure4.16 �������, ��Ӧ������ primBounds ���� nodeBounds �������ཻ��???
    if (bestAxis == -1 && retries < 2) 
    {
        ++retries;
        axis = (axis + 1) % 3;
        goto retrySplit;
    }

    // ������л��ֵĿ���(bestCost)��Ĭ�ϵĿ���(oldCost)��Ҫ��, ��������, ��Ϊ�ں����ݹ����������Ĺ����п��ܻ����ɸ��õĽ��
    // ����Ҫ��¼һ��������������Ĵ���
    if (bestCost > oldCost) ++badRefines;

    // ���ڵ�����̫��ʱ, ��������Ҷ�ڵ�����
    // �ֻ������µݹ�Ĺ�����, ������л������ǱȲ����ֵĿ�������(badRefines == 3), �Ͳ��ٻ���, ����ֱ������Ҷ�ڵ�
    if ((bestCost > 4 * oldCost && nPrimitives < 16) || bestAxis == -1 ||
        badRefines == 3) 
    {
        nodes[nodeNum].InitLeaf(primNums, nPrimitives, &primitiveIndices);
        return;
    }

    // Classify primitives with respect to split
    // �����������ӽڵ��ཻ��ͼԪ����, ���������Ӧ���� KdTreeAccel::primitives �� allPrimBounds
    int n0 = 0, n1 = 0;

    for (int i = 0;              i < bestOffset;      ++i)
        if (edges[bestAxis][i].type == EdgeType::Start)
            prims0[n0++] = edges[bestAxis][i].primNum;

    for (int i = bestOffset + 1; i < 2 * nPrimitives; ++i)
        if (edges[bestAxis][i].type == EdgeType::End)
            prims1[n1++] = edges[bestAxis][i].primNum;

    // Recursively initialize children nodes
    // ֱ�Ӷ� nodeBounds ���ֲ����µ� subNodeBounds
    Float tSplit = edges[bestAxis][bestOffset].t;
    Bounds3f bounds0 = nodeBounds, bounds1 = nodeBounds;
    bounds0.pMax[bestAxis] = bounds1.pMin[bestAxis] = tSplit;

    // ����������ʱ, ��Ȼ����� primNums �� prims0 ��ͬһ��, ���� 'Classify primitives with respect to split' ʱ, primNums �Ѿ�������, ���Ը��ǵ���������
    // ������������, ��ʼ����������ʱ, ����Ҫ�õ� prims1 ������, prims1 �����ݲ��ܱ�����, ���Դ������ 'prims1 + nPrimitives'

    // 'nodeNum + 1' -> ����ӽڵ���ڸ��ڵ�, ����Ҫ�����¼, ����һ���ӽڵ�, ����Ҫ 'InitInterior' ��¼һ����
    buildTree(nodeNum + 1, bounds0, allPrimBounds, prims0, n0, depth - 1, edges,
              prims0, prims1 + nPrimitives, badRefines); // ע�����ﴫ����� 'prims1 + nPrimitives'

    int aboveChild = nextFreeNode; // ��ȱ�����ǰһ���ڵ��, �����µ� nextFreeNode value
    nodes[nodeNum].InitInterior(bestAxis, aboveChild, tSplit);

    buildTree(aboveChild,  bounds1, allPrimBounds, prims1, n1, depth - 1, edges,
              prims0, prims1 + nPrimitives, badRefines);
}



bool KdTreeAccel::Intersect(const Ray &ray, SurfaceInteraction *isect) const 
{
    ProfilePhase p(Prof::AccelIntersect);

    // �ο� Figure 4.17 ���ĸ�����, ��ϸ��ע ray �� [tMin, tMax] �ı仯, ͨ�� [tMin, tMax] ���Ը���Ľ�������

    // Compute initial parametric range of ray inside kd-tree extent
    Float tMin, tMax;
    if (!bounds.IntersectP(ray, &tMin, &tMax)) {
        return false;
    }

    // Prepare to traverse kd-tree for ray
    Vector3f invDir(1 / ray.d.x, 1 / ray.d.y, 1 / ray.d.z);
    PBRT_CONSTEXPR int maxTodo = 64;
    KdToDo todo[maxTodo]; // ����һ��ͨ������ģ��� stack, ���ԶԱ�һ�� 'BVHAccel::Intersect'
    int todoPos = 0;

    // Traverse kd-tree nodes in order for ray
    bool hit = false;
    const KdAccelNode *node = &nodes[0];
    
    // ������ while ѭ��ʱ, �����ȵ��ɱ���������, ������ݹ�, ����, ��ֹ������, Ȼ���ٿ��� kdtree �Ŀռ�ṹ

    while (node != nullptr) 
    {
        // Bail out if we found a hit closer than the current node
        // ����Ѿ��ҵ���һ���ȵ�ǰ���ʽڵ㻹Ҫ���Ľ���, �Ͳ���Ҫ�ٲ�����, ����ѭ��
        if (ray.tMax < tMin) 
            break;

        if (!node->IsLeaf()) // ������м�ڵ�, �ͼ���������������ӽڵ�, ���µݹ�, ֱ���ҵ��������Ҷ�ڵ�Ϊֹ
        {
            // Process kd-tree interior node

            // Compute parametric distance along ray to split plane
            int axis = node->SplitAxis();
            Float tPlane = (node->SplitPos() - ray.o[axis]) * invDir[axis];

            // Get node children pointers for ray
            const KdAccelNode *firstChild, *secondChild;

            // �ο� Figure 4.18, �ȷ��ʸ������Ǹ��ӽڵ�
            int belowFirst =
                (ray.o[axis] < node->SplitPos()) ||
                (ray.o[axis] == node->SplitPos() && ray.d[axis] <= 0); // ������ԭ���ڻ���ƽ����, ����ݹ��߷�����ѡ��

            if (belowFirst) 
            {
                firstChild = node + 1;
                secondChild = &nodes[node->AboveChild()];
            } 
            else 
            {
                firstChild = &nodes[node->AboveChild()];
                secondChild = node + 1;
            }

            // Advance to next child node, possibly enqueue other child
            // �ο� Figure4.19, ǰ��������¾Ͳ���Ҫ���Ƿ�����һ���ڵ���, �������һ���ڵ�ӵ� todoList ��
            if (tPlane > tMax || tPlane <= 0)
                node = firstChild;
            else if (tPlane < tMin)
                node = secondChild;
            else 
            {
                // Enqueue _secondChild_ in todo list
                todo[todoPos].node = secondChild;
                // �ݴ� secondChild �� [tMin, tMax], Ȼ����µ���ڵ�� tMax
                todo[todoPos].tMin = tPlane;
                todo[todoPos].tMax = tMax;
                ++todoPos;

                node = firstChild;
                tMax = tPlane;
            }
        } 
        else // ��Ҷ�ڵ�Ļ�, ����������ڵ���ص�ͼԪ, һ���ҵ����ཻ��ͼԪ, �͸��� ray.tMax, ���ܲ�������ѭ��, ������ݵ�������м�ڵ�ȥ
        {
            // Check for intersections inside leaf node
            int nPrimitives = node->nPrimitives();
            if (nPrimitives == 1) 
            {
                const std::shared_ptr<Primitive> &p =
                    primitives[node->onePrimitive];

                // Check one primitive inside leaf node
                if (p->Intersect(ray, isect)) 
                    hit = true;
            } 
            else 
            {
                for (int i = 0; i < nPrimitives; ++i) 
                {
                    int index =
                        primitiveIndices[node->primitiveIndicesOffset + i];
                    const std::shared_ptr<Primitive> &p = primitives[index];

                    // Check one primitive inside leaf node
                    if (p->Intersect(ray, isect)) 
                        hit = true;
                }
            }

            // Grab next node to process from todo list
            if (todoPos > 0) 
            {
                --todoPos;
                node = todo[todoPos].node;

                // ��ʼ������һ�ڵ�ǰ, �ȸ��� ray �ڸý׶ε� [tMin, tMax]
                tMin = todo[todoPos].tMin;
                tMax = todo[todoPos].tMax;
            } 
            else
                break;
        }
    }

    return hit;
}

bool KdTreeAccel::IntersectP(const Ray &ray) const {
    ProfilePhase p(Prof::AccelIntersectP);
    // Compute initial parametric range of ray inside kd-tree extent
    Float tMin, tMax;
    if (!bounds.IntersectP(ray, &tMin, &tMax)) {
        return false;
    }

    // Prepare to traverse kd-tree for ray
    Vector3f invDir(1 / ray.d.x, 1 / ray.d.y, 1 / ray.d.z);
    PBRT_CONSTEXPR int maxTodo = 64;
    KdToDo todo[maxTodo];
    int todoPos = 0;
    const KdAccelNode *node = &nodes[0];
    while (node != nullptr) {
        if (node->IsLeaf()) {
            // Check for shadow ray intersections inside leaf node
            int nPrimitives = node->nPrimitives();
            if (nPrimitives == 1) {
                const std::shared_ptr<Primitive> &p =
                    primitives[node->onePrimitive];
                if (p->IntersectP(ray)) {
                    return true;
                }
            } else {
                for (int i = 0; i < nPrimitives; ++i) {
                    int primitiveIndex =
                        primitiveIndices[node->primitiveIndicesOffset + i];
                    const std::shared_ptr<Primitive> &prim =
                        primitives[primitiveIndex];
                    if (prim->IntersectP(ray)) {
                        return true;
                    }
                }
            }

            // Grab next node to process from todo list
            if (todoPos > 0) {
                --todoPos;
                node = todo[todoPos].node;
                tMin = todo[todoPos].tMin;
                tMax = todo[todoPos].tMax;
            } else
                break;
        } else {
            // Process kd-tree interior node

            // Compute parametric distance along ray to split plane
            int axis = node->SplitAxis();
            Float tPlane = (node->SplitPos() - ray.o[axis]) * invDir[axis];

            // Get node children pointers for ray
            const KdAccelNode *firstChild, *secondChild;
            int belowFirst =
                (ray.o[axis] < node->SplitPos()) ||
                (ray.o[axis] == node->SplitPos() && ray.d[axis] <= 0);
            if (belowFirst) {
                firstChild = node + 1;
                secondChild = &nodes[node->AboveChild()];
            } else {
                firstChild = &nodes[node->AboveChild()];
                secondChild = node + 1;
            }

            // Advance to next child node, possibly enqueue other child
            if (tPlane > tMax || tPlane <= 0)
                node = firstChild;
            else if (tPlane < tMin)
                node = secondChild;
            else {
                // Enqueue _secondChild_ in todo list
                todo[todoPos].node = secondChild;
                todo[todoPos].tMin = tPlane;
                todo[todoPos].tMax = tMax;
                ++todoPos;
                node = firstChild;
                tMax = tPlane;
            }
        }
    }
    return false;
}



std::shared_ptr<KdTreeAccel> CreateKdTreeAccelerator(
    std::vector<std::shared_ptr<Primitive>> prims, const ParamSet &ps) 
{
    int isectCost = ps.FindOneInt("intersectcost", 80);
    int travCost = ps.FindOneInt("traversalcost", 1);
    Float emptyBonus = ps.FindOneFloat("emptybonus", 0.5f);
    int maxPrims = ps.FindOneInt("maxprims", 1);
    int maxDepth = ps.FindOneInt("maxdepth", -1);
    return std::make_shared<KdTreeAccel>(std::move(prims), isectCost, travCost, emptyBonus,
                                         maxPrims, maxDepth);
}

}  // namespace pbrt
