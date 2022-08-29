project(BULLET)
include(GNUInstallDirs)

set(Bullet_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/Bullet-C-Api.h
        ${CMAKE_CURRENT_SOURCE_DIR}/btBulletCollisionCommon.h
        ${CMAKE_CURRENT_SOURCE_DIR}/btBulletDynamicsCommon.h
)
install(FILES ${Bullet_HEADERS}
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine/BULLET/")

set(Bullet_HEADERS_CollisionBroadphaseCollision
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/BroadphaseCollision/btAxisSweep3.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/BroadphaseCollision/btBroadphaseInterface.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/BroadphaseCollision/btBroadphaseProxy.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/BroadphaseCollision/btCollisionAlgorithm.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/BroadphaseCollision/btDbvt.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/BroadphaseCollision/btDbvtBroadphase.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/BroadphaseCollision/btDispatcher.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/BroadphaseCollision/btMultiSapBroadphase.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/BroadphaseCollision/btOverlappingPairCache.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/BroadphaseCollision/btOverlappingPairCallback.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/BroadphaseCollision/btQuantizedBvh.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/BroadphaseCollision/btSimpleBroadphase.h
        )
install(FILES ${Bullet_HEADERS_CollisionBroadphaseCollision}
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine/BULLET/BulletCollision/BroadphaseCollision/")

set(Bullet_HEADERS_DynamicsCollisionDispatch
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionDispatch/SphereTriangleDetector.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionDispatch/btActivatingCollisionAlgorithm.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionDispatch/btBox2dBox2dCollisionAlgorithm.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionDispatch/btBoxBoxCollisionAlgorithm.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionDispatch/btBoxBoxDetector.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionDispatch/btCollisionConfiguration.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionDispatch/btCollisionCreateFunc.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionDispatch/btCollisionDispatcher.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionDispatch/btCollisionObject.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionDispatch/btCollisionObjectWrapper.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionDispatch/btCollisionWorld.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionDispatch/btCompoundCollisionAlgorithm.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionDispatch/btCompoundCompoundCollisionAlgorithm.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionDispatch/btConvex2dConvex2dAlgorithm.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionDispatch/btConvexConcaveCollisionAlgorithm.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionDispatch/btConvexConvexAlgorithm.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionDispatch/btConvexPlaneCollisionAlgorithm.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionDispatch/btEmptyCollisionAlgorithm.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionDispatch/btGhostObject.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionDispatch/btHashedSimplePairCache.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionDispatch/btInternalEdgeUtility.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionDispatch/btManifoldResult.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionDispatch/btSimulationIslandManager.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionDispatch/btSphereBoxCollisionAlgorithm.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionDispatch/btSphereSphereCollisionAlgorithm.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionDispatch/btSphereTriangleCollisionAlgorithm.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionDispatch/btUnionFind.h
        )
install(FILES ${Bullet_HEADERS_DynamicsCollisionDispatch}
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine/BULLET/BulletCollision/CollisionDispatch/")

set(Bullet_HEADERS_DynamicsCollisionShapes
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btBox2dShape.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btBoxShape.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btCapsuleShape.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btCollisionMargin.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btCollisionShape.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btCompoundShape.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btConcaveShape.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btConeShape.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btConvex2dShape.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btConvexHullShape.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btConvexInternalShape.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btConvexPointCloudShape.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btConvexPolyhedron.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btConvexShape.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btConvexTriangleMeshShape.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btCylinderShape.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btEmptyShape.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btMaterial.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btMinkowskiSumShape.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btMultiSphereShape.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btMultimaterialTriangleMeshShape.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btOptimizedBvh.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btPolyhedralConvexShape.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btShapeHull.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btSphereShape.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btStaticPlaneShape.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btStridingMeshInterface.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btTetrahedronShape.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btTriangleBuffer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btTriangleCallback.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btTriangleIndexVertexArray.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btTriangleIndexVertexMaterialArray.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btTriangleInfoMap.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btTriangleMesh.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btTriangleMeshShape.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btTriangleShape.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/CollisionShapes/btUniformScalingShape.h
        )
install(FILES ${Bullet_HEADERS_DynamicsCollisionShapes}
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine/BULLET/BulletCollision/CollisionShapes/")


set(Bullet_HEADERS_DynamicsGimpact
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/Gimpact/btBoxCollision.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/Gimpact/btClipPolygon.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/Gimpact/btCompoundFromGimpact.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/Gimpact/btContactProcessing.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/Gimpact/btGImpactBvh.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/Gimpact/btGImpactCollisionAlgorithm.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/Gimpact/btGImpactMassUtil.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/Gimpact/btGImpactQuantizedBvh.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/Gimpact/btGImpactShape.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/Gimpact/btGenericPoolAllocator.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/Gimpact/btGeometryOperations.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/Gimpact/btQuantization.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/Gimpact/btTriangleShapeEx.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/Gimpact/gim_array.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/Gimpact/gim_basic_geometry_operations.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/Gimpact/gim_bitset.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/Gimpact/gim_box_collision.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/Gimpact/gim_box_set.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/Gimpact/gim_clip_polygon.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/Gimpact/gim_contact.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/Gimpact/gim_geom_types.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/Gimpact/gim_geometry.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/Gimpact/gim_hash_table.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/Gimpact/gim_linear_math.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/Gimpact/gim_math.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/Gimpact/gim_memory.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/Gimpact/gim_radixsort.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/Gimpact/gim_tri_collision.h
        )
install(FILES ${Bullet_HEADERS_DynamicsGimpact}
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine/BULLET/BulletCollision/Gimpact/")


set(Bullet_HEADERS_DynamicsNarrowPhaseCollision
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/NarrowPhaseCollision/btContinuousConvexCollision.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/NarrowPhaseCollision/btConvexCast.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/NarrowPhaseCollision/btConvexPenetrationDepthSolver.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/NarrowPhaseCollision/btDiscreteCollisionDetectorInterface.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/NarrowPhaseCollision/btGjkConvexCast.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/NarrowPhaseCollision/btGjkEpa2.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/NarrowPhaseCollision/btGjkEpaPenetrationDepthSolver.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/NarrowPhaseCollision/btGjkPairDetector.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/NarrowPhaseCollision/btManifoldPoint.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/NarrowPhaseCollision/btMinkowskiPenetrationDepthSolver.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/NarrowPhaseCollision/btPersistentManifold.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/NarrowPhaseCollision/btPointCollector.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/NarrowPhaseCollision/btPolyhedralContactClipping.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/NarrowPhaseCollision/btRaycastCallback.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/NarrowPhaseCollision/btSimplexSolverInterface.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/NarrowPhaseCollision/btSubSimplexConvexCast.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletCollision/NarrowPhaseCollision/btVoronoiSimplexSolver.h
        )
install(FILES ${Bullet_HEADERS_DynamicsNarrowPhaseCollision}
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine/BULLET/BulletCollision/NarrowPhaseCollision/")


set(Bullet_HEADERS_DynamicsCharacter
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/Character/btCharacterControllerInterface.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/Character/btKinematicCharacterController.h
        )
install(FILES ${Bullet_HEADERS_DynamicsCharacter}
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine/BULLET/BulletDynamics/Character/")


set(Bullet_HEADERS_DynamicsConstraintSolver
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/ConstraintSolver/btConeTwistConstraint.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/ConstraintSolver/btConstraintSolver.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/ConstraintSolver/btContactConstraint.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/ConstraintSolver/btContactSolverInfo.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/ConstraintSolver/btFixedConstraint.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/ConstraintSolver/btGearConstraint.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/ConstraintSolver/btGeneric6DofConstraint.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/ConstraintSolver/btGeneric6DofSpringConstraint.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/ConstraintSolver/btHinge2Constraint.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/ConstraintSolver/btHingeConstraint.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/ConstraintSolver/btJacobianEntry.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/ConstraintSolver/btPoint2PointConstraint.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/ConstraintSolver/btSliderConstraint.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/ConstraintSolver/btSolve2LinearConstraint.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/ConstraintSolver/btSolverBody.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/ConstraintSolver/btSolverConstraint.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/ConstraintSolver/btTypedConstraint.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/ConstraintSolver/btUniversalConstraint.h
        )
install(FILES ${Bullet_HEADERS_DynamicsConstraintSolver}
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine/BULLET/BulletDynamics/ConstraintSolver/")


set(Bullet_HEADERS_DynamicsDyn
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/Dynamics/btActionInterface.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/Dynamics/btDynamicsWorld.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/Dynamics/btRigidBody.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/Dynamics/btSimpleDynamicsWorld.h
        )
install(FILES ${Bullet_HEADERS_DynamicsDyn}
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine/BULLET/BulletDynamics/Dynamics/")


set(Bullet_HEADERS_DynamicsFeatherstone
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/Featherstone/btMultiBody.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/Featherstone/btMultiBodyConstraint.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/Featherstone/btMultiBodyConstraintSolver.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/Featherstone/btMultiBodyDynamicsWorld.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/Featherstone/btMultiBodyJointLimitConstraint.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/Featherstone/btMultiBodyJointMotor.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/Featherstone/btMultiBodyLink.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/Featherstone/btMultiBodyLinkCollider.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/Featherstone/btMultiBodyPoint2Point.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/Featherstone/btMultiBodySolverConstraint.h
        )
install(FILES ${Bullet_HEADERS_DynamicsFeatherstone}
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine/BULLET/BulletDynamics/Featherstone/")


set(Bullet_HEADERS_DynamicsMLCPSolvers
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/MLCPSolvers/btDantzigLCP.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/MLCPSolvers/btDantzigSolver.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/MLCPSolvers/btMLCPSolver.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/MLCPSolvers/btMLCPSolverInterface.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/MLCPSolvers/btPATHSolver.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/MLCPSolvers/btSolveProjectedGaussSeidel.h
        )
install(FILES ${Bullet_HEADERS_DynamicsMLCPSolvers}
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine/BULLET/BulletDynamics/MLCPSolvers/")


set(Bullet_HEADERS_DynamicsVehicle
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/Vehicle/btRaycastVehicle.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/Vehicle/btVehicleRaycaster.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletDynamics/Vehicle/btWheelInfo.h
        )
install(FILES ${Bullet_HEADERS_DynamicsVehicle}
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine/BULLET/BulletDynamics/Vehicle/")


set(Bullet_HEADERS_SoftBody
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletSoftBody/btDefaultSoftBodySolver.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletSoftBody/btSoftBody.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletSoftBody/btSoftBodyConcaveCollisionAlgorithm.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletSoftBody/btSoftBodyData.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletSoftBody/btSoftBodyHelpers.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletSoftBody/btSoftBodyInternals.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletSoftBody/btSoftBodySolverVertexBuffer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletSoftBody/btSoftBodySolvers.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletSoftBody/btSoftRigidCollisionAlgorithm.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletSoftBody/btSoftRigidDynamicsWorld.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletSoftBody/btSoftSoftCollisionAlgorithm.h
        ${CMAKE_CURRENT_SOURCE_DIR}/BulletSoftBody/btSparseSDF.h
        )
install(FILES ${Bullet_HEADERS_SoftBody}
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine/BULLET/BulletSoftBody/")


set(Bullet_HEADERS_LinearMath
        ${CMAKE_CURRENT_SOURCE_DIR}/LinearMath/btAabbUtil2.h
        ${CMAKE_CURRENT_SOURCE_DIR}/LinearMath/btAlignedAllocator.h
        ${CMAKE_CURRENT_SOURCE_DIR}/LinearMath/btAlignedObjectArray.h
        ${CMAKE_CURRENT_SOURCE_DIR}/LinearMath/btConvexHull.h
        ${CMAKE_CURRENT_SOURCE_DIR}/LinearMath/btConvexHullComputer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/LinearMath/btDefaultMotionState.h
        ${CMAKE_CURRENT_SOURCE_DIR}/LinearMath/btGeometryUtil.h
        ${CMAKE_CURRENT_SOURCE_DIR}/LinearMath/btGrahamScan2dConvexHull.h
        ${CMAKE_CURRENT_SOURCE_DIR}/LinearMath/btHashMap.h
        ${CMAKE_CURRENT_SOURCE_DIR}/LinearMath/btIDebugDraw.h
        ${CMAKE_CURRENT_SOURCE_DIR}/LinearMath/btList.h
        ${CMAKE_CURRENT_SOURCE_DIR}/LinearMath/btMatrix3x3.h
        ${CMAKE_CURRENT_SOURCE_DIR}/LinearMath/btMatrixX.h
        ${CMAKE_CURRENT_SOURCE_DIR}/LinearMath/btMinMax.h
        ${CMAKE_CURRENT_SOURCE_DIR}/LinearMath/btMotionState.h
        ${CMAKE_CURRENT_SOURCE_DIR}/LinearMath/btPolarDecomposition.h
        ${CMAKE_CURRENT_SOURCE_DIR}/LinearMath/btPoolAllocator.h
        ${CMAKE_CURRENT_SOURCE_DIR}/LinearMath/btQuadWord.h
        ${CMAKE_CURRENT_SOURCE_DIR}/LinearMath/btQuaternion.h
        ${CMAKE_CURRENT_SOURCE_DIR}/LinearMath/btQuickprof.h
        ${CMAKE_CURRENT_SOURCE_DIR}/LinearMath/btRandom.h
        ${CMAKE_CURRENT_SOURCE_DIR}/LinearMath/btScalar.h
        ${CMAKE_CURRENT_SOURCE_DIR}/LinearMath/btSerializer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/LinearMath/btStackAlloc.h
        ${CMAKE_CURRENT_SOURCE_DIR}/LinearMath/btTransform.h
        ${CMAKE_CURRENT_SOURCE_DIR}/LinearMath/btTransformUtil.h
        ${CMAKE_CURRENT_SOURCE_DIR}/LinearMath/btVector3.h
        )
install(FILES ${Bullet_HEADERS_LinearMath}
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine/BULLET/LinearMath/")


add_library(Bullet STATIC
    BulletCollision/BroadphaseCollision/btAxisSweep3.cpp
    BulletCollision/BroadphaseCollision/btBroadphaseProxy.cpp
    BulletCollision/BroadphaseCollision/btCollisionAlgorithm.cpp
    BulletCollision/BroadphaseCollision/btDbvt.cpp
    BulletCollision/BroadphaseCollision/btDbvtBroadphase.cpp
    BulletCollision/BroadphaseCollision/btDispatcher.cpp
    BulletCollision/BroadphaseCollision/btMultiSapBroadphase.cpp
    BulletCollision/BroadphaseCollision/btOverlappingPairCache.cpp
    BulletCollision/BroadphaseCollision/btQuantizedBvh.cpp
    BulletCollision/BroadphaseCollision/btSimpleBroadphase.cpp
    BulletCollision/CollisionDispatch/SphereTriangleDetector.cpp
    BulletCollision/CollisionDispatch/btActivatingCollisionAlgorithm.cpp
    BulletCollision/CollisionDispatch/btBox2dBox2dCollisionAlgorithm.cpp
    BulletCollision/CollisionDispatch/btBoxBoxCollisionAlgorithm.cpp
    BulletCollision/CollisionDispatch/btBoxBoxDetector.cpp
    BulletCollision/CollisionDispatch/btCollisionDispatcher.cpp
    BulletCollision/CollisionDispatch/btCollisionObject.cpp
    BulletCollision/CollisionDispatch/btCollisionWorld.cpp
    BulletCollision/CollisionDispatch/btCompoundCollisionAlgorithm.cpp
    BulletCollision/CollisionDispatch/btCompoundCompoundCollisionAlgorithm.cpp
    BulletCollision/CollisionDispatch/btConvex2dConvex2dAlgorithm.cpp
    BulletCollision/CollisionDispatch/btConvexConcaveCollisionAlgorithm.cpp
    BulletCollision/CollisionDispatch/btConvexConvexAlgorithm.cpp
    BulletCollision/CollisionDispatch/btConvexPlaneCollisionAlgorithm.cpp
    BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.cpp
    BulletCollision/CollisionDispatch/btEmptyCollisionAlgorithm.cpp
    BulletCollision/CollisionDispatch/btGhostObject.cpp
    BulletCollision/CollisionDispatch/btHashedSimplePairCache.cpp
    BulletCollision/CollisionDispatch/btInternalEdgeUtility.cpp
    BulletCollision/CollisionDispatch/btManifoldResult.cpp
    BulletCollision/CollisionDispatch/btSimulationIslandManager.cpp
    BulletCollision/CollisionDispatch/btSphereBoxCollisionAlgorithm.cpp
    BulletCollision/CollisionDispatch/btSphereSphereCollisionAlgorithm.cpp
    BulletCollision/CollisionDispatch/btSphereTriangleCollisionAlgorithm.cpp
    BulletCollision/CollisionDispatch/btUnionFind.cpp
    BulletCollision/CollisionShapes/btBox2dShape.cpp
    BulletCollision/CollisionShapes/btBoxShape.cpp
    BulletCollision/CollisionShapes/btBvhTriangleMeshShape.cpp
    BulletCollision/CollisionShapes/btCapsuleShape.cpp
    BulletCollision/CollisionShapes/btCollisionShape.cpp
    BulletCollision/CollisionShapes/btCompoundShape.cpp
    BulletCollision/CollisionShapes/btConcaveShape.cpp
    BulletCollision/CollisionShapes/btConeShape.cpp
    BulletCollision/CollisionShapes/btConvex2dShape.cpp
    BulletCollision/CollisionShapes/btConvexHullShape.cpp
    BulletCollision/CollisionShapes/btConvexInternalShape.cpp
    BulletCollision/CollisionShapes/btConvexPointCloudShape.cpp
    BulletCollision/CollisionShapes/btConvexPolyhedron.cpp
    BulletCollision/CollisionShapes/btConvexShape.cpp
    BulletCollision/CollisionShapes/btConvexTriangleMeshShape.cpp
    BulletCollision/CollisionShapes/btCylinderShape.cpp
    BulletCollision/CollisionShapes/btEmptyShape.cpp
    BulletCollision/CollisionShapes/btHeightfieldTerrainShape.cpp
    BulletCollision/CollisionShapes/btMinkowskiSumShape.cpp
    BulletCollision/CollisionShapes/btMultiSphereShape.cpp
    BulletCollision/CollisionShapes/btMultimaterialTriangleMeshShape.cpp
    BulletCollision/CollisionShapes/btOptimizedBvh.cpp
    BulletCollision/CollisionShapes/btPolyhedralConvexShape.cpp
    BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.cpp
    BulletCollision/CollisionShapes/btShapeHull.cpp
    BulletCollision/CollisionShapes/btSphereShape.cpp
    BulletCollision/CollisionShapes/btStaticPlaneShape.cpp
    BulletCollision/CollisionShapes/btStridingMeshInterface.cpp
    BulletCollision/CollisionShapes/btTetrahedronShape.cpp
    BulletCollision/CollisionShapes/btTriangleBuffer.cpp
    BulletCollision/CollisionShapes/btTriangleCallback.cpp
    BulletCollision/CollisionShapes/btTriangleIndexVertexArray.cpp
    BulletCollision/CollisionShapes/btTriangleIndexVertexMaterialArray.cpp
    BulletCollision/CollisionShapes/btTriangleMesh.cpp
    BulletCollision/CollisionShapes/btTriangleMeshShape.cpp
    BulletCollision/CollisionShapes/btUniformScalingShape.cpp
    BulletCollision/Gimpact/btContactProcessing.cpp
    BulletCollision/Gimpact/btGImpactBvh.cpp
    BulletCollision/Gimpact/btGImpactCollisionAlgorithm.cpp
    BulletCollision/Gimpact/btGImpactQuantizedBvh.cpp
    BulletCollision/Gimpact/btGImpactShape.cpp
    BulletCollision/Gimpact/btGenericPoolAllocator.cpp
    BulletCollision/Gimpact/btTriangleShapeEx.cpp
    BulletCollision/Gimpact/gim_box_set.cpp
    BulletCollision/Gimpact/gim_contact.cpp
    BulletCollision/Gimpact/gim_memory.cpp
    BulletCollision/Gimpact/gim_tri_collision.cpp
    BulletCollision/NarrowPhaseCollision/btContinuousConvexCollision.cpp
    BulletCollision/NarrowPhaseCollision/btConvexCast.cpp
    BulletCollision/NarrowPhaseCollision/btGjkConvexCast.cpp
    BulletCollision/NarrowPhaseCollision/btGjkEpa2.cpp
    BulletCollision/NarrowPhaseCollision/btGjkEpaPenetrationDepthSolver.cpp
    BulletCollision/NarrowPhaseCollision/btGjkPairDetector.cpp
    BulletCollision/NarrowPhaseCollision/btMinkowskiPenetrationDepthSolver.cpp
    BulletCollision/NarrowPhaseCollision/btPersistentManifold.cpp
    BulletCollision/NarrowPhaseCollision/btPolyhedralContactClipping.cpp
    BulletCollision/NarrowPhaseCollision/btRaycastCallback.cpp
    BulletCollision/NarrowPhaseCollision/btSubSimplexConvexCast.cpp
    BulletCollision/NarrowPhaseCollision/btVoronoiSimplexSolver.cpp
    BulletDynamics/Character/btKinematicCharacterController.cpp
    BulletDynamics/ConstraintSolver/btConeTwistConstraint.cpp
    BulletDynamics/ConstraintSolver/btContactConstraint.cpp
    BulletDynamics/ConstraintSolver/btFixedConstraint.cpp
    BulletDynamics/ConstraintSolver/btGearConstraint.cpp
    BulletDynamics/ConstraintSolver/btGeneric6DofConstraint.cpp
    BulletDynamics/ConstraintSolver/btGeneric6DofSpringConstraint.cpp
    BulletDynamics/ConstraintSolver/btHinge2Constraint.cpp
    BulletDynamics/ConstraintSolver/btHingeConstraint.cpp
    BulletDynamics/ConstraintSolver/btPoint2PointConstraint.cpp
    BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.cpp
    BulletDynamics/ConstraintSolver/btSliderConstraint.cpp
    BulletDynamics/ConstraintSolver/btSolve2LinearConstraint.cpp
    BulletDynamics/ConstraintSolver/btTypedConstraint.cpp
    BulletDynamics/ConstraintSolver/btUniversalConstraint.cpp
    BulletDynamics/Dynamics/Bullet-C-API.cpp
    BulletDynamics/Dynamics/btDiscreteDynamicsWorld.cpp
    BulletDynamics/Dynamics/btRigidBody.cpp
    BulletDynamics/Dynamics/btSimpleDynamicsWorld.cpp
    BulletDynamics/Featherstone/btMultiBody.cpp
    BulletDynamics/Featherstone/btMultiBodyConstraint.cpp
    BulletDynamics/Featherstone/btMultiBodyConstraintSolver.cpp
    BulletDynamics/Featherstone/btMultiBodyDynamicsWorld.cpp
    BulletDynamics/Featherstone/btMultiBodyJointLimitConstraint.cpp
    BulletDynamics/Featherstone/btMultiBodyJointMotor.cpp
    BulletDynamics/Featherstone/btMultiBodyPoint2Point.cpp
    BulletDynamics/MLCPSolvers/btDantzigLCP.cpp
    BulletDynamics/MLCPSolvers/btMLCPSolver.cpp
    BulletDynamics/Vehicle/btRaycastVehicle.cpp
    BulletDynamics/Vehicle/btWheelInfo.cpp
    BulletSoftBody/btDefaultSoftBodySolver.cpp
    BulletSoftBody/btSoftBody.cpp
    BulletSoftBody/btSoftBodyConcaveCollisionAlgorithm.cpp
    BulletSoftBody/btSoftBodyHelpers.cpp
    BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.cpp
    BulletSoftBody/btSoftRigidCollisionAlgorithm.cpp
    BulletSoftBody/btSoftRigidDynamicsWorld.cpp
    BulletSoftBody/btSoftSoftCollisionAlgorithm.cpp
    LinearMath/btAlignedAllocator.cpp
    LinearMath/btConvexHull.cpp
    LinearMath/btConvexHullComputer.cpp
    LinearMath/btGeometryUtil.cpp
    LinearMath/btPolarDecomposition.cpp
    LinearMath/btQuickprof.cpp
    LinearMath/btSerializer.cpp
    LinearMath/btVector3.cpp
    ${Bullet_HEADERS}
    ${Bullet_HEADERS_CollisionBroadphaseCollision}
    ${Bullet_HEADERS_DynamicsCollisionDispatch}
    ${Bullet_HEADERS_DynamicsCollisionShapes}
    ${Bullet_HEADERS_DynamicsGimpact}
    ${Bullet_HEADERS_DynamicsNarrowPhaseCollision}
    ${Bullet_HEADERS_DynamicsCharacter}
    ${Bullet_HEADERS_DynamicsConstraintSolver}
    ${Bullet_HEADERS_DynamicsDyn}
    ${Bullet_HEADERS_DynamicsFeatherstone}
    ${Bullet_HEADERS_DynamicsMLCPSolvers}
    ${Bullet_HEADERS_DynamicsVehicle}
    ${Bullet_HEADERS_SoftBody}
    ${Bullet_HEADERS_LinearMath}
)

target_include_directories(Bullet SYSTEM PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<INSTALL_INTERFACE:include/WickedEngine/BULLET>
)

set_target_properties(Bullet PROPERTIES
	FOLDER "ThirdParty"
	POSITION_INDEPENDENT_CODE ${WICKED_DYNAMIC_LIBRARY}
)
