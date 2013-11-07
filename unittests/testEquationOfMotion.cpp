/*
 * Copyright (c) 2011, Georgia Tech Research Corporation
 * All rights reserved.
 *
 * Author(s): Jeongseok Lee <jslee02@gmail.com>
 * Date: 05/23/2013
 *
 * Geoorgia Tech Graphics Lab and Humanoid Robotics Lab
 *
 * Directed by Prof. C. Karen Liu and Prof. Mike Stilman
 * <karenliu@cc.gatech.edu> <mstilman@cc.gatech.edu>
 *
 * This file is provided under the following "BSD-style" License:
 *   Redistribution and use in source and binary forms, with or
 *   without modification, are permitted provided that the following
 *   conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 *   CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 *   USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 *   AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *   POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>

#include <Eigen/LU>
#include <gtest/gtest.h>

#include "TestHelpers.h"

#include "common/Console.h"
#include "math/Geometry.h"
#include "math/Helpers.h"
#include "dynamics/BallJoint.h"
#include "dynamics/FreeJoint.h"
#include "dynamics/PrismaticJoint.h"
#include "dynamics/RevoluteJoint.h"
#include "dynamics/Skeleton.h"
#include "dynamics/TranslationalJoint.h"
#include "dynamics/UniversalJoint.h"
#include "dynamics/WeldJoint.h"
#include "dynamics/EulerJoint.h"
#include "dynamics/ScrewJoint.h"
#include "simulation/World.h"
#include "utils/Paths.h"
#include "utils/SkelParser.h"

using namespace dart;
using namespace math;
using namespace dynamics;

#define EOM_TOL 0.01

/******************************************************************************/
class EOM : public testing::Test
{
public:
    void equationsOfMotionTest(const std::string& _fileName);
};

/******************************************************************************/
void EOM::equationsOfMotionTest(const std::string& _fileName)
{
    bool debugPrint = false;

    bool isRandomControlInput = false;
    double lowerControlInput = -0.001;
    double upperControlInput = 0.001;

    bool isRandomExternalForce = false;
    double lowerExternalForce = -0.001;
    double upperExternalForce = 0.001;

    bool isJointDamping = false;
    double dampingCoeff = 0.001;

    simulation::World* myWorld = utils::SkelParser::readSkelFile(_fileName);
    EXPECT_TRUE(myWorld != NULL);

    myWorld->setGravity(Eigen::Vector3d(0.0, 0.0, -9.81));
    myWorld->setTimeStep(0.001);

    double simTime = 10.000;
    double timeStep = myWorld->getTimeStep();
    int nSteps = simTime / timeStep;

    if (isJointDamping)
    {
        for (int i = 0; i < myWorld->getNumSkeletons(); ++i)
        {
            dynamics::Skeleton* skeleton = myWorld->getSkeleton(i);
            for (int j = 0; j < skeleton->getNumBodyNodes(); ++j)
            {
                dynamics::BodyNode* bodyNode = skeleton->getBodyNode(j);
                dynamics::Joint* joint = bodyNode->getParentJoint();

                for (int k = 0; k < joint->getNumGenCoords(); ++k)
                    joint->setDampingCoefficient(dampingCoeff, k);
            }
        }
    }

    // Step forward
    for (int i = 0; i < nSteps; i++)
    {
        myWorld->step();

        for (int j = 0; j < myWorld->getNumSkeletons(); ++j)
        {
            dynamics::Skeleton* skeleton = myWorld->getSkeleton(j);
            int n = skeleton->getNumGenCoords();

            // Mass matrix
            Eigen::MatrixXd M_OLD = skeleton->getMassMatrix_OLD();
            Eigen::MatrixXd M = skeleton->getMassMatrix();
            if (debugPrint && !equals(M_OLD, M))
            {
                std::cout << "M_OLD: " << M_OLD << std::endl;
                std::cout << "M    : " << M << std::endl;
            }
            EXPECT_TRUE(equals(M_OLD, M));

            // Inverse mass matrix
            Eigen::MatrixXd MInv_OLD       = skeleton->getInvMassMatrix_OLD();
            Eigen::MatrixXd MInv           = skeleton->getInvMassMatrix();
            Eigen::MatrixXd I              = Eigen::MatrixXd::Identity(n,n);
            Eigen::MatrixXd M_MInv         = MInv * M;
            Eigen::MatrixXd MInv_M         = M * MInv;
            Eigen::MatrixXd MInv_M_OLD     = MInv * M_OLD;
            Eigen::MatrixXd M_OLD_MInv     = M_OLD * MInv;
            Eigen::MatrixXd MInv_OLD_M     = MInv_OLD * M;
            Eigen::MatrixXd M_MInv_OLD     = M * MInv_OLD;
            Eigen::MatrixXd MInv_OLD_M_OLD = MInv_OLD * M_OLD;
            Eigen::MatrixXd M_OLD_MInv_OLD = M_OLD* MInv_OLD;
//            EXPECT_TRUE(equals(MInv_OLD_M_OLD, I));
//            EXPECT_TRUE(equals(M_OLD_MInv_OLD, I));
            if (equals(MInv_OLD_M_OLD, I) && equals(M_OLD_MInv_OLD, I))
            {
                EXPECT_TRUE(equals(M_MInv, I));
                EXPECT_TRUE(equals(MInv_M, I));
//                EXPECT_TRUE(equals(MInv_M_OLD, I));
//                EXPECT_TRUE(equals(M_OLD_MInv, I));
//                EXPECT_TRUE(equals(MInv_OLD_M, I));
//                EXPECT_TRUE(equals(M_MInv_OLD, I));

                if (debugPrint &&
                    (!equals(M_MInv, I) ||
                     !equals(MInv_M, I) ||
                     !equals(MInv_M_OLD, I) ||
                     !equals(M_OLD_MInv, I) ||
                     !equals(MInv_OLD_M, I) ||
                     !equals(M_MInv_OLD, I)))
                {
                    std::cout << "M_OLD: \n" << M_OLD << std::endl;
                    std::cout << "M    : \n" << M << std::endl;
                    std::cout << "MInv_OLD: \n" << MInv_OLD << std::endl;
                    std::cout << "MInv    : \n" << MInv << std::endl;
                    std::cout << "Diff    : \n" << MInv - MInv_OLD << std::endl;
                    std::cout << "M * MInv: \n" << M * MInv << std::endl;
                    std::cout << "Psi     : \n" << skeleton->getBodyNode(0)->mPsi << std::endl;
                    std::cout << "AI      : \n" << skeleton->getBodyNode(0)->mAI << std::endl;
                    std::cout << "S       : \n" << skeleton->getBodyNode(0)->getParentJoint()->getLocalJacobian() << std::endl;
                    std::cout << "S^T AI S: \n" << skeleton->getBodyNode(0)->getParentJoint()->getLocalJacobian().transpose() * skeleton->getBodyNode(0)->mAI * skeleton->getBodyNode(0)->getParentJoint()->getLocalJacobian() << std::endl;
                    std::cout << "(S^T AI S)^{-1}: \n" << (skeleton->getBodyNode(0)->getParentJoint()->getLocalJacobian().transpose() * skeleton->getBodyNode(0)->mAI * skeleton->getBodyNode(0)->getParentJoint()->getLocalJacobian()).inverse() << std::endl;
                }
            }

            // Coriolis and gravity force vector
            Eigen::VectorXd Cg_OLD = skeleton->getCombinedVector_OLD();
            Eigen::VectorXd Cg     = skeleton->getCombinedVector();
            if (debugPrint && !equals(Cg_OLD, Cg))
            {
                std::cout << "Cg_OLD: \n" << Cg_OLD << std::endl;
                std::cout << "Cg    : \n" << Cg << std::endl;
            }
            EXPECT_TRUE(equals(Cg_OLD, Cg));

            // Coriolis force vector
            Eigen::VectorXd C_OLD = skeleton->getCoriolisForceVector_OLD();
            Eigen::VectorXd C     = skeleton->getCoriolisForceVector();
            if (debugPrint && !equals(C_OLD, C))
            {
                std::cout << "C_OLD: \n" << C_OLD << std::endl;
                std::cout << "C    : \n" << C << std::endl;
            }
            EXPECT_TRUE(equals(C_OLD, C));

            // Gravity force vector
            Eigen::VectorXd g_OLD = skeleton->getGravityForceVector_OLD();
            Eigen::VectorXd g     = skeleton->getGravityForceVector();
            if (debugPrint && !equals(g_OLD, g))
            {
                std::cout << "g_OLD: \n" << g_OLD << std::endl;
                std::cout << "g    : \n" << g << std::endl;
            }
            EXPECT_TRUE(equals(g_OLD, g));

            // Control input
            if (isRandomControlInput)
            {
                Eigen::VectorXd Fint = skeleton->getInternalForceVector();
                for (int k = 0; k < Fint.size(); ++k)
                    Fint[k] += math::random(lowerControlInput, upperControlInput);
                skeleton->setInternalForceVector(Fint);
            }

            // External force
            if (isRandomExternalForce)
            {
                for (int k = 0; k < skeleton->getNumBodyNodes(); ++k)
                {
                    Eigen::Vector3d force;
                    Eigen::Vector3d torque;
                    for (int l = 0; l < 3; ++l)
                    {
                        force[l] = math::random(lowerExternalForce, upperExternalForce);
                        torque[l] = math::random(lowerExternalForce, upperExternalForce);
                    }
                }
            }

            // External force vector
            Eigen::VectorXd Fext_OLD = skeleton->getExternalForceVector_OLD();
            Eigen::VectorXd Fext     = skeleton->getExternalForceVector();
            if (debugPrint && !equals(Fext_OLD, Fext))
            {
                std::cout << "Fext_OLD: \n" << Fext_OLD << std::endl;
                std::cout << "Fext    : \n" << Fext << std::endl;
            }
            EXPECT_TRUE(equals(Fext_OLD, Fext));
        }
    }

    // Random state test
    int nRandomTest = 1000;
    for (int i = 0; i < nRandomTest; i++)
    {
        for (int j = 0; j < myWorld->getNumSkeletons(); ++j)
        {
            dynamics::Skeleton* skeleton = myWorld->getSkeleton(j);
            int n = skeleton->getNumGenCoords();

            // Random state
            Eigen::VectorXd state = skeleton->getState();
            for (int j = 0; j < state.size(); ++j)
            {
                state[j] = math::random(-DART_PI*2.0, DART_PI*2.0);
//                state[j] = math::random(-DART_PI*0.9, DART_PI*0.9);
                state[j] = math::random(-DART_PI*0.4, DART_PI*0.4);
//                state[j] = DART_PI*0.5;
            }
            skeleton->setState(state);

            // Mass matrix
            Eigen::MatrixXd M_OLD = skeleton->getMassMatrix_OLD();
            Eigen::MatrixXd M = skeleton->getMassMatrix();
            if (debugPrint && !equals(M_OLD, M))
            {
                std::cout << "M_OLD: " << M_OLD << std::endl;
                std::cout << "M    : " << M << std::endl;
            }
            EXPECT_TRUE(equals(M_OLD, M));

            // Inverse mass matrix
            Eigen::MatrixXd MInv_OLD       = skeleton->getInvMassMatrix_OLD();
            Eigen::MatrixXd MInv           = skeleton->getInvMassMatrix();
            Eigen::MatrixXd I              = Eigen::MatrixXd::Identity(n,n);
            Eigen::MatrixXd M_MInv         = MInv * M;
            Eigen::MatrixXd MInv_M         = M * MInv;
            Eigen::MatrixXd MInv_M_OLD     = MInv * M_OLD;
            Eigen::MatrixXd M_OLD_MInv     = M_OLD * MInv;
            Eigen::MatrixXd MInv_OLD_M     = MInv_OLD * M;
            Eigen::MatrixXd M_MInv_OLD     = M * MInv_OLD;
            Eigen::MatrixXd MInv_OLD_M_OLD = MInv_OLD * M_OLD;
            Eigen::MatrixXd M_OLD_MInv_OLD = M_OLD* MInv_OLD;
//            EXPECT_TRUE(equals(MInv_OLD_M_OLD, I));
//            EXPECT_TRUE(equals(M_OLD_MInv_OLD, I));
            if (equals(MInv_OLD_M_OLD, I) && equals(M_OLD_MInv_OLD, I))
            {
                EXPECT_TRUE(equals(M_MInv, I));
                EXPECT_TRUE(equals(MInv_M, I));
//                EXPECT_TRUE(equals(MInv_M_OLD, I));
//                EXPECT_TRUE(equals(M_OLD_MInv, I));
//                EXPECT_TRUE(equals(MInv_OLD_M, I));
//                EXPECT_TRUE(equals(M_MInv_OLD, I));

//                if (!equals(M_MInv, I) ||
//                    !equals(MInv_M, I) ||
//                    !equals(MInv_M_OLD, I) ||
//                    !equals(M_OLD_MInv, I) ||
//                    !equals(MInv_OLD_M, I) ||
//                    !equals(M_MInv_OLD, I))
//                {
//                    Eigen::FullPivLU<Eigen::MatrixXd> lu(M_OLD);
//                    std::cout << "The rank of M_OLD is" << lu.rank() << std::endl;
//                    if (lu.isInvertible())
//                    {
//                        std::cout << "A is invertible, its inverse is:"
//                                  << std::endl << lu.inverse() << std::endl;
//                    }
//                    else
//                    {
//                        std::cout << "Here's a matrix whose columns form a basis of the kernel a.k.a. nullspace of A:"
//                                  << std::endl << lu.kernel() << std::endl;
//                    }
//                }

                if (debugPrint &&
                    (!equals(M_MInv, I) ||
                     !equals(MInv_M, I) ||
                     !equals(MInv_M_OLD, I) ||
                     !equals(M_OLD_MInv, I) ||
                     !equals(MInv_OLD_M, I) ||
                     !equals(M_MInv_OLD, I)))
                {
                    std::cout << "M_OLD: \n" << M_OLD << std::endl;
                    std::cout << "M    : \n" << M << std::endl;
                    std::cout << "MInv_OLD: \n" << MInv_OLD << std::endl;
                    std::cout << "MInv    : \n" << MInv << std::endl;
                    std::cout << "Diff    : \n" << MInv - MInv_OLD << std::endl;
                    std::cout << "M * MInv: \n" << M * MInv << std::endl;
                    std::cout << "Psi     : \n" << skeleton->getBodyNode(0)->mPsi << std::endl;
                    std::cout << "AI      : \n" << skeleton->getBodyNode(0)->mAI << std::endl;
                    std::cout << "S       : \n" << skeleton->getBodyNode(0)->getParentJoint()->getLocalJacobian() << std::endl;
                    std::cout << "S^T AI S: \n" << skeleton->getBodyNode(0)->getParentJoint()->getLocalJacobian().transpose() * skeleton->getBodyNode(0)->mAI * skeleton->getBodyNode(0)->getParentJoint()->getLocalJacobian() << std::endl;
                    std::cout << "(S^T AI S)^{-1}: \n" << (skeleton->getBodyNode(0)->getParentJoint()->getLocalJacobian().transpose() * skeleton->getBodyNode(0)->mAI * skeleton->getBodyNode(0)->getParentJoint()->getLocalJacobian()).inverse() << std::endl;
                }
            }

            // Coriolis and gravity force vector
            Eigen::VectorXd Cg_OLD = skeleton->getCombinedVector_OLD();
            Eigen::VectorXd Cg     = skeleton->getCombinedVector();
            if (debugPrint && !equals(Cg_OLD, Cg))
            {
                std::cout << "Cg_OLD: \n" << Cg_OLD << std::endl;
                std::cout << "Cg    : \n" << Cg << std::endl;
            }
            EXPECT_TRUE(equals(Cg_OLD, Cg));

            // Coriolis force vector
            Eigen::VectorXd C_OLD = skeleton->getCoriolisForceVector_OLD();
            Eigen::VectorXd C     = skeleton->getCoriolisForceVector();
            if (debugPrint && !equals(C_OLD, C))
            {
                std::cout << "C_OLD: \n" << C_OLD << std::endl;
                std::cout << "C    : \n" << C << std::endl;
            }
            EXPECT_TRUE(equals(C_OLD, C));

            // Gravity force vector
            Eigen::VectorXd g_OLD = skeleton->getGravityForceVector_OLD();
            Eigen::VectorXd g     = skeleton->getGravityForceVector();
            if (debugPrint && !equals(g_OLD, g))
            {
                std::cout << "g_OLD: \n" << g_OLD << std::endl;
                std::cout << "g    : \n" << g << std::endl;
            }
            EXPECT_TRUE(equals(g_OLD, g));

            // Control input
            if (isRandomControlInput)
            {
                Eigen::VectorXd Fint = skeleton->getInternalForceVector();
                for (int k = 0; k < Fint.size(); ++k)
                    Fint[k] += math::random(lowerControlInput, upperControlInput);
                skeleton->setInternalForceVector(Fint);
            }

            // External force
            if (isRandomExternalForce)
            {
                for (int k = 0; k < skeleton->getNumBodyNodes(); ++k)
                {
                    Eigen::Vector3d force;
                    Eigen::Vector3d torque;
                    for (int l = 0; l < 3; ++l)
                    {
                        force[l] = math::random(lowerExternalForce, upperExternalForce);
                        torque[l] = math::random(lowerExternalForce, upperExternalForce);
                    }
                }
            }

            // External force vector
            Eigen::VectorXd Fext_OLD = skeleton->getExternalForceVector_OLD();
            Eigen::VectorXd Fext     = skeleton->getExternalForceVector();
            if (debugPrint && !equals(Fext_OLD, Fext))
            {
                std::cout << "Fext_OLD: \n" << Fext_OLD << std::endl;
                std::cout << "Fext    : \n" << Fext << std::endl;
            }
            EXPECT_TRUE(equals(Fext_OLD, Fext));
        }
    }
}

/******************************************************************************/
TEST_F(EOM, EquationOfMotion)
{
    dtdbg << "single_pendulum.skel" << std::endl;
    equationsOfMotionTest(DART_DATA_PATH"/skel/test/single_pendulum.skel");

    dtdbg << "single_pendulum_euler_joint.skel" << std::endl;
    equationsOfMotionTest(DART_DATA_PATH"/skel/test/single_pendulum_euler_joint.skel");

    dtdbg << "single_pendulum_ball_joint.skel" << std::endl;
    equationsOfMotionTest(DART_DATA_PATH"/skel/test/single_pendulum_ball_joint.skel");

    dtdbg << "double_pendulum.skel" << std::endl;
    equationsOfMotionTest(DART_DATA_PATH"/skel/test/double_pendulum.skel");

    dtdbg << "double_pendulum_euler_joint.skel" << std::endl;
    equationsOfMotionTest(DART_DATA_PATH"/skel/test/double_pendulum_euler_joint.skel");

    dtdbg << "double_pendulum_ball_joint.skel" << std::endl;
    equationsOfMotionTest(DART_DATA_PATH"/skel/test/double_pendulum_ball_joint.skel");

    dtdbg << "serial_chain_revolute_joint.skel" << std::endl;
    equationsOfMotionTest(DART_DATA_PATH"/skel/test/serial_chain_revolute_joint.skel");

    dtdbg << "serial_chain_eulerxyz_joint.skel" << std::endl;
    equationsOfMotionTest(DART_DATA_PATH"/skel/test/serial_chain_eulerxyz_joint.skel");

    dtdbg << "serial_chain_ball_joint.skel" << std::endl;
    equationsOfMotionTest(DART_DATA_PATH"/skel/test/serial_chain_ball_joint.skel");

    dtdbg << "simple_tree_structure.skel" << std::endl;
    equationsOfMotionTest(DART_DATA_PATH"/skel/test/simple_tree_structure.skel");

    dtdbg << "simple_tree_structure_euler_joint.skel" << std::endl;
    equationsOfMotionTest(DART_DATA_PATH"/skel/test/simple_tree_structure_euler_joint.skel");

    dtdbg << "simple_tree_structure_ball_joint.skel" << std::endl;
    equationsOfMotionTest(DART_DATA_PATH"/skel/test/simple_tree_structure_ball_joint.skel");

    dtdbg << "tree_structure.skel" << std::endl;
    equationsOfMotionTest(DART_DATA_PATH"/skel/test/tree_structure.skel");

    dtdbg << "tree_structure_euler_joint.skel" << std::endl;
    equationsOfMotionTest(DART_DATA_PATH"/skel/test/tree_structure_euler_joint.skel");

    dtdbg << "tree_structure_ball_joint.skel" << std::endl;
    equationsOfMotionTest(DART_DATA_PATH"/skel/test/tree_structure_ball_joint.skel");

    dtdbg << "fullbody1.skel" << std::endl;
    equationsOfMotionTest(DART_DATA_PATH"/skel/fullbody1.skel");
}

/******************************************************************************/
int main(int argc, char* argv[])
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}