
# @author Shin'ichiro Nakaoka

#set(CMAKE_BUILD_TYPE Debug)
#set_source_files_properties(SceneBody.cpp PROPERTIES COMPILE_FLAGS "-O0 -g")

set(target CnoidBody)

set(sources
  Body.cpp
  VRMLBody.cpp
  LinkTraverse.cpp
  Link.cpp
  LinkPath.cpp
  JointPath.cpp
  Jacobian.cpp
  Device.cpp
  ForceSensor.cpp
  RateGyroSensor.cpp
  AccelerationSensor.cpp
  BasicSensorSimulationHelper.cpp
  Camera.cpp
  RangeCamera.cpp
  RangeSensor.cpp
  Light.cpp
  PointLight.cpp
  SpotLight.cpp
  MultiDeviceStateSeq.cpp
  ExtraBodyStateAccessor.cpp
  SceneBody.cpp
  SceneDevice.cpp
  SceneCollision.cpp
  CompositeIK.cpp
  PinDragIK.cpp
  LinkGroup.cpp
  LeggedBodyHelper.cpp
  BodyCollisionDetectorUtil.cpp
  BodyMotion.cpp
  BodyMotionPoseProvider.cpp
  BodyState.cpp
  BodyCustomizerInterface.cpp
  ZMPSeq.cpp
  ForwardDynamics.cpp
  ForwardDynamicsABM.cpp
  ForwardDynamicsCBM.cpp
  DyBody.cpp
  DyWorld.cpp
  MassMatrix.cpp
  ConstraintForceSolver.cpp
  InverseDynamics.cpp
  PenetrationBlocker.cpp
  AbstractBodyLoader.cpp
  BodyLoader.cpp
  YAMLBodyLoader.cpp
  VRMLBodyLoader.cpp
  VRMLBodyWriter.cpp
  ColladaBodyLoader.cpp
  PoseProviderToBodyMotionConverter.cpp
  BodyMotionUtil.cpp
  )

set(headers
  Body.h
  VRMLBody.h
  BodyCustomizerInterface.h
  AbstractBodyLoader.h
  VRMLBodyLoader.h
  ColladaBodyLoader.h
  BodyLoader.h
  VRMLBodyWriter.h
  ZMPSeq.h
  Link.h
  LinkTraverse.h
  LinkPath.h
  JointPath.h
  LinkGroup.h
  BodyCollisionDetectorUtil.h
  MultiDeviceStateSeq.h
  Device.h
  DeviceList.h
  ForceSensor.h
  RateGyroSensor.h
  AccelerationSensor.h
  BasicSensorSimulationHelper.h
  Camera.h
  RangeCamera.h
  RangeSensor.h
  Light.h
  PointLight.h
  SpotLight.h
  SceneBody.h
  SceneDevice.h
  SceneCollision.h
  InverseKinematics.h
  CompositeIK.h
  PinDragIK.h
  LeggedBodyHelper.h
  PenetrationBlocker.h
  ForwardDynamics.h
  ForwardDynamicsABM.h
  ForwardDynamicsCBM.h
  DyBody.h
  DyWorld.h
  InverseDynamics.h
  Jacobian.h
  MassMatrix.h
  ContactAttribute.h
  ConstraintForceSolver.h
  PoseProvider.h
  BodyMotion.h
  BodyMotionPoseProvider.h
  PoseProviderToBodyMotionConverter.h
  BodyMotionUtil.h
  BodyState.h
  exportdecl.h
  gettext.h
  CollisionLinkPair.h
  )


make_gettext_mofiles(${target} mofiles)
add_cnoid_library(${target} SHARED ${sources} ${headers} ${mofiles})

if(UNIX)
  target_link_libraries(${target} CnoidUtil CnoidAISTCollisionDetector dl)
elseif(MSVC)
  target_link_libraries(${target} CnoidUtil CnoidAISTCollisionDetector)
endif()

apply_common_setting_for_library(${target} "${headers}")


function(add_cnoid_body_customizer)

  set(target ${ARGV0})
  list(REMOVE_AT ARGV 0)
  add_library(${target} SHARED ${ARGV})

  set_target_properties(${target} PROPERTIES
    PREFIX ""
    COMPILE_DEFINITIONS "CNOID_BODY_CUSTOMIZER"
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${CNOID_PLUGIN_SUBDIR}/customizer)

  if(ENABLE_INSTALL_RPATH)
    set_target_properties(${target} PROPERTIES INSTALL_RPATH "$ORIGIN")
  endif()

  if(QT5)
    qt5_use_modules(${target} Widgets)
  endif()

  install(TARGETS ${target}
    RUNTIME DESTINATION ${CNOID_PLUGIN_SUBDIR}/customizer CONFIGURATIONS Release Debug RelWithDebInfo MinSizeRel
    LIBRARY DESTINATION ${CNOID_PLUGIN_SUBDIR}/customizer CONFIGURATIONS Release Debug RelWithDebInfo MinSizeRel)

endfunction()


# Body Customizers
set(BODY_CUSTOMIZERS ${BODY_CUSTOMIZERS} CACHE FILEPATH "Source files of body customizers")

if(BODY_CUSTOMIZERS)
  foreach(src ${BODY_CUSTOMIZERS})
    get_filename_component(customizer ${src} NAME_WE)
    add_cnoid_body_customizer(${customizer} ${src})
  endforeach()
endif()

install_external_libraries(${Boost_LIBRARY_DIRS} ${Boost_LIBRARY_DIRS})

if(ENABLE_PYTHON)
  add_subdirectory(python)
endif()


