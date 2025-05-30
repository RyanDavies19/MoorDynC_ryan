/*
 * Copyright (c) 2022, Matt Hall
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/** @file Line.hpp
 * C++ API for the moordyn::Line object
 */

#pragma once

#include "Misc.hpp"
#include "Instance.hpp"
#include "Seafloor.hpp"
#include "Util/CFL.hpp"
#include <utility>

#ifdef USE_VTK
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#endif

using namespace std;

namespace moordyn {

class Waves;
typedef std::shared_ptr<Waves> WavesRef;

/** @class Line Line.hpp
 * @brief A mooring line
 *
 * Each mooring line is divided on a set of nodes connected by segments, with
 * two points at the end points
 *
 * [point (node 0)] - seg 0 - [node 1] - ... - seg n-1 - [point (node N)]
 *
 * Depending on the length of the line, \f$ l \f$, the number of segments,
 * \f$ n \f$, and the material density and Young's modulus, \f$ \rho \f$ &
 * \f$ E \f$, a natural oscillation period can be defined:
 *
 * \f$ T = \frac{l}{\pi n} \sqrt{\frac{\rho}{E}} \f$
 *
 * The integration time step (moordyn::MoorDyn.dtM0) should be smaller than
 * this natural period to avoid numerical instabilities
 */
class DECLDIR Line final
  : public Instance
  , public NatFreqCFL
{
  public:
	/** @brief Constructor
	 * @param log Logging handler
	 * @param lineId the incremental Id of the line
	 */
	Line(moordyn::Log* log, size_t lineId);

	/** @brief Destructor
	 */
	~Line();

  private:
	/** @brief Get the non-linear stiffness. This is interpolated from a
	 * curve provided in the input file.
	 * @param l_stretched The actual length of the segment
	 * @param l_unstretched The unstretched length of the segment
	 */
	real getNonlinearEA(real l_stretched, real l_unstretched) const;

	/** @brief Get the non-linear Damping coefficient
	 * @param ld_stretched The segment rate of stretch
	 * @param l_unstretched The unstretched length of the segment
	 */
	real getNonlinearBA(real ld_stretched, real l_unstretched) const;

	/** @brief Get the non-linear bending stiffness
	 * @param curv The curvature
	 */
	real getNonlinearEI(real curv) const;

	/** @brief Finds the depth of the water at some (x, y) point. Either using
	 * env->WtrDpth or the 3D seafloor if available
	 * @param x x coordinate
	 * @param y y coordinate
	 * @return A negative number representing the sea floor depth at the given
	 * location
	 */
	inline real getWaterDepth(real x, real y)
	{
		return seafloor ? seafloor->getDepthAt(x, y) : -env->WtrDpth;
	}

	/** @brief A single value representing the average water depth
	 *
	 */
	inline real avgWaterDepth()
	{
		return seafloor ? seafloor->getAverageDepth() : -env->WtrDpth;
	}
	// ENVIRONMENTAL STUFF
	/// Global struct that holds environmental settings
	EnvCondRef env;
	/// global Waves object
	moordyn::WavesRef waves;
	/// Object containing the 3d seafloor info
	moordyn::SeafloorRef seafloor;

	/// Number of line segments
	unsigned int N;
	/// Unstretched line length
	moordyn::real UnstrLen;
	/// Unstretched line length at the beginning of the time step
	moordyn::real UnstrLen0;
	/// Unstretched line length rate of change
	moordyn::real UnstrLend;
	/// line diameter
	moordyn::real d;
	/// line density (kg/m^3)
	moordyn::real rho;
	/// Elasticity model flag (1: constant EA, 2: viscoelastic model with
	/// constant dynamic stiffness, 3: mean load depenedent dynamic stiffness)
	unsigned int ElasticMod;
	/// line normal/static elasticity modulus * crosssectional area [N]
	moordyn::real EA;
	/// constant line dynamic stiffness modulus * area for viscoelastic stuff
	/// [N]
	moordyn::real EA_D;
	/// Alpha * MBL in load dependent dynamic stiffness equation Kd = Alpha *
	/// MBL + vbeta * Lm for viscoelastic model
	moordyn::real alphaMBL;
	/// beta in load dependent dynamic stiffness equation Kd = Alpha * MBL +
	/// vbeta * Lm for viscoelastic model
	moordyn::real vbeta;
	/// stiffness of spring 2 in viscoelastic model (dynamic stiffness). This is
	/// the spring in series with the parallel spring-dashpot. if ElasticMod =
	/// 2, EA_2 = EA_D. If ElasticMod = 3, EA_2 is load dependent dynamic
	/// stiffness.
	moordyn::real EA_2;
	/// segment stretch attributed to static stiffness portion [m] (deltaL_1)
	std::vector<moordyn::real> dl_1;
	/// rate of change of static stiffness portion [m/s] (Ldot_1)
	std::vector<moordyn::real> ld_1;
	/// line bending stiffness [Nm^2]
	moordyn::real EI;
	/** line axial internal damping coefficient [Pa-s]
	 *
	 * The provided value in moordyn::Line::props::BA can be either the literal
	 * damping coefficient, or a negative value in which case it represents the
	 * fraction of the critical damping * area,
	 *
	 * \f$ C_{crit} = \frac{l}{n} \sqrt{\rho E} \f$
	 */
	moordyn::real BA;
	/// Line axial damping coefficient corresponding to the dynamic spring in
	/// the viscoelastic model
	moordyn::real BA_D;
	/// normal added mass coefficient [-]
	/// with respect to line displacement
	moordyn::real Can;
	/// axial added mass coefficient [-]
	/// with respect to line displacement
	moordyn::real Cat;
	/// normal drag coefficient [-]
	/// with respect to frontal area, \f$ d l \f$
	moordyn::real Cdn;
	/// axial drag coefficient [-]
	/// with respect to surface area, \f$ \pi d l \f$
	moordyn::real Cdt;
	/// VIV lift coefficient [-]
	/// If 0, VIV turned off
	moordyn::real Cl;
	/// +- range of VIV synchronization [-]
	/// in non-dimensional frequency
	moordyn::real dF;
	/// Center VIV synchronization [-]
	/// in non-dimensional frequency
	moordyn::real cF;

	/// line axial internal damping coefficient (before proceessing) [Ns]
	moordyn::real BAin;
	/// line cross-sectional area to pre-compute [m2]
	moordyn::real A;
	/// number of values in stress-strain lookup table (0 for constant E)
	unsigned int nEApoints;
	/// x array for stress-strain lookup table
	std::vector<moordyn::real> stiffXs;
	/// y array for stress-strain lookup table
	std::vector<moordyn::real> stiffYs;
	/// number of values in bent stiffness lookup table (0 for constant EI)
	unsigned int nEIpoints;
	/// x array for bent stiffness lookup table
	std::vector<moordyn::real> bstiffXs;
	/// y array for bent stiffness lookup table
	std::vector<moordyn::real> bstiffYs;
	/// number of values in stress-strainrate lookup table (0 for constant c)
	unsigned int nBApoints;
	/// x array for stress-strainrate lookup table
	std::vector<moordyn::real> dampXs;
	/// y array for stress-strainrate lookup table
	std::vector<moordyn::real> dampYs;

	// Externally provided data
	/** true if pressure bending forces shall be considered, false otherwise
	 * @see
	 * https://cds.cern.ch/record/1224245/files/PH-EP-Tech-Note-2009-004.pdf?version=1
	 */
	bool isPb;
	/// internal pipe pressure at the nodes (Pa)
	std::vector<moordyn::real> pin;

	// kinematics
	/// node positions
	std::vector<vec> r;
	/// node velocities
	std::vector<vec> rd;
	/// unit tangent vectors for each node
	std::vector<vec> q;
	/// unit normal vectors for each node (used in bending calcs)
	std::vector<vec> pvec;
	/// unit tangent vectors for each segment (used in bending calcs)
	std::vector<vec> qs;
	/// unstretched line segment lengths
	std::vector<moordyn::real> l;
	/// stretched segment lengths
	std::vector<moordyn::real> lstr;
	/// rate of stretch
	std::vector<moordyn::real> ldstr;
	/// curvatures at node points (1/m)
	std::vector<moordyn::real> Kurv;

	/// node mass + added mass matrix
	std::vector<mat> M;
	// line segment volumes
	std::vector<moordyn::real> V;

	// forces
	/// segment tensions
	std::vector<vec> T;
	/// segment damping forces
	std::vector<vec> Td;
	/// bending stiffness forces
	std::vector<vec> Bs;
	/// Pressure bending forces
	std::vector<vec> Pb;
	/// node weights
	std::vector<vec> W;
	/// node drag (transversal)
	std::vector<vec> Dp;
	/// node drag (axial)
	std::vector<vec> Dq;
	/// node added mass forcing (transversal)
	std::vector<vec> Ap;
	/// node added mass forcing (axial)
	std::vector<vec> Aq;
	/// node bottom contact force
	std::vector<vec> B;
	/// crossflow viv lift force
	std::vector<vec> Lf;
	/// total force on node
	std::vector<vec> Fnet;

	// wave things
	/// VOF scalar for each segment (1 = fully submerged, 0 = out of water)
	std::vector<moordyn::real> F;

	// time
	/// simulation time
	moordyn::real t;
	/// MoorDyn internal time step
	moordyn::real dtm0;

	// VIV stuff
	// /// VIV amplitude updated every zero crossing of crossflow velcoity
	// std::vector<moordyn::real> Amp;
	/// Num timesteps for rolling RMS of crossflow velocity phase
	const unsigned int n_m = 500;
	/// The crossflow motion phase [-]
	real phi_yd;
	/// The phase of the crossflow force
	std::vector<moordyn::real> phi;
	/// The frequency of the lift force (rad/s) (dphi / dt)
	std::vector<moordyn::real> phi_dot;

	// back indexing one dtm for VIV
	/// old t
	moordyn::real t_old = 0.0;
	// /// running amplitude total, from previous zero crossing of yd
	// std::vector<moordyn::real> A_int_old;
	/// node old cf vel rms
	std::vector<moordyn::real> yd_rms_old;
	/// node old cf accel rms
	std::vector<moordyn::real> ydd_rms_old;
	/// node old accelerations
	std::vector<vec> rdd_old;

	// end conditions
	/** @brief Types of end points
	 */
	typedef enum
	{
		/// Pinned to Point
		PINNED = 0,
		/// Cantilevered to Rod
		CANTILEVERED = 1,
		// Some aliases
		POINT = PINNED,
		ROD = CANTILEVERED,
	} endTypes;

	/// type of point at end A: 0=pinned to Point, 1=cantilevered to
	/// Rod.
	endTypes endTypeA;
	/// type of point at end B: 0=pinned to Point, 1=cantilevered to
	/// Rod.
	endTypes endTypeB;
	/// moment at end A from bending, to be applied on attached Rod/Body
	vec endMomentA;
	/// moment at end A from bending, to be applied on attached Rod/Body
	vec endMomentB;

	// file stuff
	/// Pointer to moordyn::MoorDyn::outfileMain
	ofstream* outfile;
	/// A copy of moordyn::MoorDyn::outChans
	string channels;

	/** data structures for precalculated nodal water kinematics if applicable
	 * @{
	 */

	/// time series of wave elevations above each node
	std::vector<std::vector<moordyn::real>> zetaTS;
	/// Volume of fluid at each node (currently unused)
	std::vector<std::vector<moordyn::real>> FTS;
	/// time series of velocities at each node
	std::vector<std::vector<vec>> UTS;
	/// time series of accelerations at each node
	std::vector<std::vector<vec>> UdTS;
	/// number of water kinematics time steps
	unsigned int ntWater;
	/// water kinematics time step size (s)
	moordyn::real dtWater;

	/**
	 * @}
	 */

  public:
	/// Line ID
	int number;
	/// a line id which is guaranteed to be contiguous up from zero for the
	/// lines
	size_t lineId;

	/// boolean to indicate dynamic relaxation occuring
	bool IC_gen = false;

	/** @brief Setup a line
	 * @param number Line number
	 * @param props Line properties
	 * @param l Unstretched line length
	 * @param n Number of segments
	 * @param env_in Global struct that holds environmental settings
	 * @param outfile The outfile where information shall be written
	 * @param channels The channels/fields that shall be printed in the file
	 * @param dtM0 moordyn internal timestep. Used in VIV
	 */
	void setup(int number,
	           LineProps* props,
	           real l,
	           unsigned int n,
	           EnvCondRef env_in,
	           shared_ptr<ofstream> outfile,
	           string channels,
	           real dtM0);

	/** @brief Set the environmental data
	 * @param waves_in Global Waves object
	 * @param seafloor_in Global 3D Seafloor object
	 */

	inline void setWaves(moordyn::WavesRef waves_in,
	                     moordyn::SeafloorRef seafloor_in)
	{
		waves = waves_in;
		seafloor = seafloor_in;
	}

	/** @brief Initialize the line object
	 * @note Becasue Line.hpp is the only function that calls this, no state
	 * vectors need to be returned. This is different from the structure of
	 * the other objects initialize functions.
	 * @throws moordyn::output_file_error If an outfile has been provided, but
	 * it cannot be written
	 * @throws invalid_value_error If there is no enough water depth
	 */
	void initialize();

	/** @brief Sets the initial line state
	 * @param state The line state for initializing (see Line::setState for the
	 * state structure0)
	 * @note This calls Line::Initialize()
	 */
	inline void initialize(InstanceStateVarView state)
	{
		// ------ Initialize the line ------
		initialize();

		// ------ Assign the intialized values to the state (bascially
		// Line::setState but flipped) ------ Error check for number of columns
		// (if VIV and Visco need row.size() = 8, if VIV xor Visco need
		// row.size() = 7, if not VIV need row.size() = 6)
		if ((state.row(0).size() != 8 && Cl > 0 && ElasticMod > 1) ||
		    (state.row(0).size() != 7 && ((Cl > 0) ^ (ElasticMod > 1))) ||
		    (state.row(0).size() != 6 && Cl == 0 && ElasticMod == 1)) {
			LOGERR << "Invalid state.row size for Line " << number << endl;
			throw moordyn::mem_error("Invalid state.row size");
		}

		// Error check for number of rows (if visco need N rows, if normal need
		// N-1 rows)
		if ((state.rows() != N && ElasticMod > 1) ||
		    (state.rows() != N - 1 && ElasticMod == 1)) {
			LOGERR << "Invalid number of rows in state matrix for Line "
			       << number << endl;
			throw moordyn::mem_error("Invalid number of rows in state matrix");
		}

		// If using the viscoelastic model, iterate N rows, else iterate N-1
		// rows.
		for (unsigned int i = 0; i < (ElasticMod > 1 ? N : N - 1); i++) {
			// node number is i+1
			// segment number is i
			state.row(i).head<3>() = r[i + 1];
			state.row(i).segment<3>(3) = rd[i + 1];

			if (ElasticMod > 1)
				state.row(i).tail<1>()[0] =
				    dl_1[i]; // [0] needed becasue tail<1> returns a one element
				             // vector. Viscoelastic state is always the last
				             // element in the row

			if (Cl > 0) {
				if (ElasticMod > 1)
					state.row(i).tail<2>()[0] =
					    phi[i + 1]; // if both VIV and viscoelastic second to
					                // last element in the row
				else
					state.row(i).tail<1>()[0] =
					    phi[i + 1]; // else last element in the row
			}
		}
	}
	/**
	 * @}
	 */

	/** @brief Number of segments
	 *
	 * The number of nodes can be computed as moordyn::Line::getN() + 1
	 * @return The number of segments, moordyn::Line::N
	 */
	inline unsigned int getN() const { return N; }

	/** @brief Get the unstretched length of the line
	 * @return The unstretched length, moordyn::Line::UnstrLen
	 */
	inline moordyn::real getUnstretchedLength() const { return UnstrLen; }

	/** @brief Set the unstretched length of the line
	 *
	 * This function is consistently changing the unstretched length of each
	 * segment, moordyn::Line::l
	 * @param len The unstretched length, moordyn::Line::UnstrLen
	 * @note This function should be called after moordyn::Line::initialize()
	 * @warning The lines damping is not changed, which might affect the
	 * stability
	 */
	inline void setUnstretchedLength(const moordyn::real len)
	{
		UnstrLen = len;
		for (unsigned int i = 0; i < N; i++) {
			l[i] = UnstrLen / double(N);
			V[i] = l[i] * A;
		}
	}

	/** @brief Set the unstretched length rate of change of the line
	 * @param v The unstretched length rate of change, moordyn::Line::UnstrLend
	 * @see moordyn::Line::setUnstretchedLength()
	 */
	inline void setUnstretchedLengthVel(const moordyn::real v)
	{
		UnstrLend = v;
	}

	/** @brief Update the unstretched length of the line, according to the
	 * velocity
	 *
	 * @param dt The time step. If zero, the initial unstretched length is set
	 * @note This function should be called after moordyn::Line::initialize()
	 * @warning The lines damping is not changed, which might affect the
	 * stability
	 */
	inline void updateUnstretchedLength(const moordyn::real dt = 0.0)
	{
		if (!UnstrLend)
			return;
		if (!dt) {
			UnstrLen0 = UnstrLen;
			return;
		}
		setUnstretchedLength(UnstrLen0 + dt * UnstrLend);
	}

	/** @brief Get whether the line is governed by a non-linear stiffness or a
	 * constant one
	 * @return true if the stiffness of the line is constant, false if a
	 * non-linear stiffness has been set
	 */
	inline bool isConstantEA() const { return nEApoints > 0; }

	/** @brief Get the constant stiffness of the line
	 *
	 * This value is useless if non-linear stiffness is considered
	 * @return The static stiffness EA value
	 * @see ::IsConstantEA()
	 */
	inline moordyn::real getConstantEA() const { return EA; }

	/** @brief Set the constant stiffness of the line
	 *
	 * This value is useless if non-linear stiffness is considered
	 * @param EA The constant axial stiffness EA value (N)
	 * @see ::IsConstantEA()
	 */
	inline void setConstantEA(moordyn::real EA_in)
	{
		if (ElasticMod > 1) {
			LOGERR << "Cannot set constant EA for viscoelastic model" << endl;
			throw moordyn::invalid_value_error(
			    "Cannot set constant EA for viscoelastic model");
		} else {
			EA = EA_in;
		}
	}

	/** @brief Get the position of a node
	 * @param i The line node index
	 * @return The position
	 * @throws invalid_value_error If the node index \p i is bigger than the
	 * number of nodes, moordyn::Line::N + 1
	 */
	inline const vec& getNodePos(unsigned int i) const
	{
		if (i > N) {
			LOGERR << "Asking node " << i << " of line " << number
			       << ", which only has " << N + 1 << " nodes" << std::endl;
			throw moordyn::invalid_value_error("Invalid node index");
		}
		if (isnan(r[i].sum())) {
			stringstream s;
			s << "NaN detected" << endl
			  << "Line " << number << " node positions:" << endl;
			for (unsigned int j = 0; j <= N; j++)
				s << j << " : " << r[j] << ";" << endl;
			throw moordyn::nan_error(s.str().c_str());
		}
		return r[i];
	}

	/** @brief Get the velocity of a node
	 * @param i The line node index
	 * @return The velocity
	 * @throws invalid_value_error If the node index \p i is bigger than the
	 * number of nodes, moordyn::Line::N + 1
	 */
	inline const vec& getNodeVel(unsigned int i) const
	{
		if (i > N) {
			LOGERR << "Asking node " << i << " of line " << number
			       << ", which only has " << N + 1 << " nodes" << std::endl;
			throw moordyn::invalid_value_error("Invalid node index");
		}
		if (isnan(rd[i].sum())) {
			stringstream s;
			s << "NaN detected" << endl
			  << "Line " << number << " node velocities:" << endl;
			for (unsigned int j = 0; j <= N; j++)
				s << j << " : " << rd[j] << ";" << endl;
			throw moordyn::nan_error(s.str().c_str());
		}
		return rd[i];
	}

	/** @brief Get the net force on a node
	 *
	 * The net force is the total force acting over a line node.
	 *
	 * To get the different components of the force use ::getNodeTen() ,
	 * ::getNodeBendStiff() , ::getNodeWeight() , ::getNodeDrag() ,
	 * ::getNodeFroudeKrilov() and ::getNodeSeaBedForce()
	 * @note The net force is \b not the sum of all the components that you
	 * cat extract from the API. For instance, the tension contribution on the
	 * internal nodes is the difference between the tensions of the adjacent
	 * segments, while ::getNodeTen() is returning the average.
	 * @param i The line node index
	 * @return The tension
	 * @throws invalid_value_error If the node index \p i is bigger than the
	 * number of nodes, moordyn::Line::N + 1
	 */
	inline const vec& getNodeForce(unsigned int i) const
	{
		if (i > N) {
			LOGERR << "Asking node " << i << " of line " << number
			       << ", which only has " << N + 1 << " nodes" << std::endl;
			throw moordyn::invalid_value_error("Invalid node index");
		}
		return Fnet[i];
	};

	/** @brief Get the tension on a node, including the internal line damping
	 *
	 * If it is an inner node, the average of the
	 * tension at the surrounding segments is provided. If the node is a
	 * line-end, the associated ending segment tension is provided. This
	 * does not account for the direction of pull. In the calculation of
	 * Fnet for the node, the direction of pull is accounted for.
	 * @param i The line node index
	 * @return The tension
	 * @throws invalid_value_error If the node index \p i is bigger than the
	 * number of nodes, moordyn::Line::N + 1
	 */
	inline const vec getNodeTen(unsigned int i) const
	{
		if (i > N) {
			LOGERR << "Asking node " << i << " of line " << number
			       << ", which only has " << N + 1 << " nodes" << std::endl;
			throw moordyn::invalid_value_error("Invalid node index");
		}
		if (i == 0) // bottom node, return ten and damping of bottom section
			return T[0] + Td[0];
		else if (i == N) // top node, return ten and damping of top section
			return T[N - 1] + Td[N - 1];
		// internal node, take average of tension in adjacent segments.
		return (0.5 * (T[i] + T[i - 1] + Td[i] + Td[i - 1]));
	};

	/** @brief Get the tension on a node, including the internal line damping
	 *
	 * If it is an inner node, the average of the
	 * tension at the surrounding segments is provided. If the node is a
	 * line-end, the associated ending segment tension is provided
	 * @param i The line node index
	 * @return The tension
	 * @throws invalid_value_error If the node index \p i is bigger than the
	 * number of nodes, moordyn::Line::N + 1
	 */
	inline const vec& getNodeBendStiff(unsigned int i) const
	{
		if (i > N) {
			LOGERR << "Asking node " << i << " of line " << number
			       << ", which only has " << N + 1 << " nodes" << std::endl;
			throw moordyn::invalid_value_error("Invalid node index");
		}
		return Bs[i];
	};

	/** @brief Get the weight and bouyancy force acting on the node
	 * @param i The line node index
	 * @return The tension
	 * @throws invalid_value_error If the node index \p i is bigger than the
	 * number of nodes, moordyn::Line::N + 1
	 */
	inline const vec& getNodeWeight(unsigned int i) const
	{
		if (i > N) {
			LOGERR << "Asking node " << i << " of line " << number
			       << ", which only has " << N + 1 << " nodes" << std::endl;
			throw moordyn::invalid_value_error("Invalid node index");
		}
		return W[i];
	};

	/** @brief Get the drag force acting on the node
	 * @param i The line node index
	 * @return The tension
	 * @throws invalid_value_error If the node index \p i is bigger than the
	 * number of nodes, moordyn::Line::N + 1
	 */
	inline const vec getNodeDrag(unsigned int i) const
	{
		if (i > N) {
			LOGERR << "Asking node " << i << " of line " << number
			       << ", which only has " << N + 1 << " nodes" << std::endl;
			throw moordyn::invalid_value_error("Invalid node index");
		}
		return Dp[i] + Dq[i];
	};

	/** @brief Get the Froude-Krilov force acting on the node
	 * @param i The line node index
	 * @return The tension
	 * @throws invalid_value_error If the node index \p i is bigger than the
	 * number of nodes, moordyn::Line::N + 1
	 */
	inline const vec getNodeFroudeKrilov(unsigned int i) const
	{
		if (i > N) {
			LOGERR << "Asking node " << i << " of line " << number
			       << ", which only has " << N + 1 << " nodes" << std::endl;
			throw moordyn::invalid_value_error("Invalid node index");
		}
		return Ap[i] + Aq[i];
	};

	/** @brief Get the sea bed reaction force acting on the node
	 *
	 * This is eventually including the friction force
	 * @param i The line node index
	 * @return The tension
	 * @throws invalid_value_error If the node index \p i is bigger than the
	 * number of nodes, moordyn::Line::N + 1
	 */
	inline const vec getNodeSeabedForce(unsigned int i) const
	{
		if (i > N) {
			LOGERR << "Asking node " << i << " of line " << number
			       << ", which only has " << N + 1 << " nodes" << std::endl;
			throw moordyn::invalid_value_error("Invalid node index");
		}
		return B[i] + Bs[i];
	};

	/** @brief Get the line curvature at a node position
	 * @param i The line node index
	 * @return The line curvature (1 / m)
	 * @throws invalid_value_error If the node index \p i is bigger than the
	 * number of nodes, moordyn::Line::N + 1
	 * @note The curvature is only computed if bending stiffness
	 * (moordyn::Line::EI) is not zero. Otherwise the curvature of every single
	 * node will be zero.
	 */
	inline const real& getNodeCurv(unsigned int i) const
	{
		if (i > N) {
			LOGERR << "Asking node " << i << " of line " << number
			       << ", which only has " << N + 1 << " nodes" << std::endl;
			throw moordyn::invalid_value_error("Invalid node index");
		}
		return Kurv[i];
	}

	/** @brief Get the mass and added mass matrix
	 * @param i The line node index
	 * @return The mass matrix
	 * @throws invalid_value_error If the node index \p i is bigger than the
	 * number of nodes, moordyn::Line::N + 1
	 */
	inline const mat& getNodeM(unsigned int i) const
	{
		if (i > N) {
			LOGERR << "Asking node " << i << " of line " << number
			       << ", which only has " << N + 1 << " nodes" << std::endl;
			throw moordyn::invalid_value_error("Invalid node index");
		}
		return M[i];
	}

	/** @brief Get the array of coordinates of all nodes along the line
	 * @return The positions array
	 */
	inline std::vector<vec> getNodeCoordinates() const { return r; }

	/** @brief Get the tensions at the fairlead and anchor in a FASTv7 friendly
	 * way
	 * @param FairHTen Horizontal tension on the fairlead
	 * @param FairVTen Vertical tension on the fairlead
	 * @param AnchHTen Horizontal tension on the anchor
	 * @param AnchVTen Vertical tension on the anchor
	 */
	inline void getFASTtens(float* FairHTen,
	                        float* FairVTen,
	                        float* AnchHTen,
	                        float* AnchVTen) const
	{
		*FairHTen = (float)(Fnet[N](Eigen::seqN(0, 2)).norm());
		*FairVTen = (float)(Fnet[N][2] + M[N](0, 0) * (-env->g));
		*AnchHTen = (float)(Fnet[0](Eigen::seqN(0, 2)).norm());
		*AnchVTen = (float)(Fnet[0][2] + M[0](0, 0) * (-env->g));
	}

	/** @brief Get the force, moment and mass at the line endpoint
	 * @param Fnet_out The output force vector
	 * @param Moment_out The output moment vector
	 * @param M_out The output mass matrix
	 * @param end_point Either ENDPOINT_TOP or ENDPOINT_BOTTOM
	 * @throws invalid_value_error If @p end_point is not a valid end point
	 * qualifier
	 */
	inline void getEndStuff(vec& Fnet_out,
	                        vec& Moment_out,
	                        mat& M_out,
	                        EndPoints end_point) const
	{
		switch (end_point) {
			case ENDPOINT_TOP:
				Fnet_out = Fnet[N];
				Moment_out = endMomentB;
				M_out = M[N];
				break;
			case ENDPOINT_BOTTOM:
				Fnet_out = Fnet[0];
				Moment_out = endMomentA;
				M_out = M[0];
				break;
			default:
				LOGERR << "Invalid end point qualifier: " << end_point << endl;
				throw moordyn::invalid_value_error("Invalid end point");
		}
	}

	/** @brief Get line output
	 *
	 * This function is useful when outputs are set in the line properties
	 * @param outChan The output channel/field
	 * @return The output value, 0.0 if a non-valid field is set
	 */
	real GetLineOutput(OutChanProps outChan);

	/** @brief store wave/current kinematics time series for this line
	 *
	 * This is used when nodal approaches are selected, i.e.
	 * WaveKin = WAVES_FFT_NODE or WAVES_NODE, Currents = CURRENTS_STEADY_NODE
	 * or CURRENTS_DYNAMIC_NODE
	 * @param dt Time step
	 * @param zeta The wave elevations
	 * @param f The fluid fractions
	 * @param u The flow velocity
	 * @param ud The flow acceleration
	 * @throws invalid_value_error If @p zeta, @p f, @p u and @p ud have not the
	 * same size, in both dimensions.
	 * @throws invalid_value_error If the length of @p zeta, @p f, @p u or @p ud
	 * is not equal to moordyn::Line::getN() + 1
	 * @note Working in progress
	 */
	void storeWaterKin(real dt,
	                   std::vector<std::vector<moordyn::real>> zeta,
	                   std::vector<std::vector<moordyn::real>> f,
	                   std::vector<std::vector<vec>> u,
	                   std::vector<std::vector<vec>> ud);

	/** @brief calculate the volume of the segment between firstNodeIdx and
	 * secondNodeIdx submerged
	 *
	 * This must be used with adjacent nodes for accurate results. It is
	 * currently only implemented for the still water case.
	 * @param firstNodeIdx Index of the first node of the segment
	 * @param secondNodeIdx Index of the second node of the segment
	 * @param surfaceHeight Height of the water surface (assumed locally
	 * horizontal)
	 */
	real calcSubSeg(unsigned int firstNodeIdx,
	                unsigned int secondNodeIdx,
	                real surfaceHeight);

	/** @brief Get the drag coefficients
	 * @return The normal (transversal) and tangential (axial) drag coefficients
	 */
	inline std::pair<real, real> getDrag() const { return make_pair(Cdn, Cdt); }

	/** @brief Set the drag coefficients
	 * @param cdn Normal (transversal) coefficient
	 * @param cdt tangential (axial) coefficient
	 */
	inline void setDrag(real cdn, real cdt)
	{
		Cdn = cdn;
		Cdt = cdt;
	}

	/** @brief Scale the drag coefficients
	 * @param scaler The drag coefficients scale factor
	 */
	inline void scaleDrag(real scaler)
	{
		Cdn = Cdn * scaler;
		Cdt = Cdt * scaler;
	}

	/** @brief Enable the pressure bending forces (disabled by default)
	 *
	 * The internal pressure can be provided at each node by calling
	 * ::internalPress(), while the external pressure is computed as the
	 * hydrostatic pressure plus the dynamic pressure (see
	 * moordyn::Waves::getWaveKinLine()).
	 *
	 * If no internal pressure is provided, zeros will be considered.
	 */
	inline void enablePb() { isPb = true; }

	/** @brief Disable the pressure bending forces (disabled by default)
	 */
	inline void disablePb() { isPb = false; }

	/** @brief Check if pressure bending forces are considered
	 * @return true if pressure bending forces are considered, false otherwise
	 */
	inline bool enabledPb() const { return isPb; }

	/** @brief Set the internal pressure at each node of the line
	 *
	 * If this is not provided, the last stored values (zero by default) will
	 * be considered.
	 *
	 * This function is useless unless ::enablePb() is called.
	 * @param p The pressure values (Pa) at each node
	 * @throws invalid_value_error If @p p has wrong size
	 */
	void setPin(std::vector<real> p);

	/** @brief Set the line  simulation time
	 * @param time Simulation time
	 */
	inline void setTime(real time) { t = time; }

	/** @brief Set the line state
	 * @param r The line state matrix. See Line::setState in Line.cpp for
	 * structure
	 * @note End node kinematics are not handled here
	 * @see moordyn::Line::setEndState
	 * @{
	 */
	void setState(const InstanceStateVarView r);

	/** @brief Set the position and velocity of an end point
	 * @param r Position
	 * @param rd Velocity
	 * @param end_point Either ENDPOINT_TOP or ENDPOINT_BOTTOM
	 * @throws invalid_value_error If @p end_point is not a valid end point
	 * qualifier
	 */
	void setEndKinematics(vec r, vec rd, EndPoints end_point);

	/** @brief set end node unit vector
	 *
	 * This method is called by an eventually attached Rod, only applicable for
	 * bending stiffness
	 * @param q The direction unit vector
	 * @param end_point Either ENDPOINT_B or ENDPOINT_A
	 * @param rod_end_point Either ENDPOINT_B or ENDPOINT_A
	 * @throws invalid_value_error If either @p end_point or @p rod_end_point
	 * are not valid end point qualifiers
	 */
	void setEndOrientation(vec q, EndPoints end_point, EndPoints rod_end_point);

	/** @brief Get the bending moment at the end point
	 *
	 * The bending moment is defined as
	 * \f$\bar{r} \frac{E I}{\vert \bar{r} \vert^2}\f$, with \f$\bar{r}\f$ the
	 * vector joining the endpoint and the next line node
	 *
	 * This method is already taking into account the line and rod end points
	 * to appropriately set the sign of the resulting moment
	 * @param end_point Either ENDPOINT_B or ENDPOINT_A
	 * @param rod_end_point Either ENDPOINT_B or ENDPOINT_A
	 * @return The moment vector
	 * @throws invalid_value_error If either @p end_point or @p rod_end_point
	 * are not valid end point qualifiers
	 */
	vec getEndSegmentMoment(EndPoints end_point, EndPoints rod_end_point) const;

	/** @brief Calculate forces and get the derivative of the line's states
	 * @param drdt Output state derivatives
	 * @throws nan_error If nan values are detected in any node position
	 */
	void getStateDeriv(InstanceStateVarView drdt);

	// void initiateStep(vector<double> &rFairIn, vector<double> &rdFairIn,
	// double time);

	void Output(real);

	/** @brief Get the number of state variables required by this instance
	 * @return The number of internal nodes if no viscoelastic model
	 * otherwise, return the number of segments
	 * @note See comments in Line::setState to see the line state structure
	 * @warning This function shall be called after ::setup()
	 */
	inline const size_t stateN() const
	{
		if (ElasticMod > 1)
			return getN(); // N rows for viscoelastic case
		else
			return getN() - 1; // N-1 rows for other cases
	}

	/** @brief Get the dimension of the state variable
	 * @return 3 components for positions and 3 components for velocities, i.e.
	 * 6 components. An additional component is returned if the VIV model or
	 * viscoelastic model is activated.
	 * @note If VIV and vsicoelastic, return 8
	 * if VIV xor viscoelastic, return 7
	 * if normal, return 6
	 * See comments in Line::setState to see the line state structure
	 * @warning This function shall be called after ::setup()
	 */
	inline const size_t stateDims() const
	{
		if (Cl > 0 && ElasticMod > 1)
			return 8; // 3 for position, 3 for velocity, 1 for VIV phase, 1 for
			          // viscoelasticity
		else if ((Cl > 0) ^ (ElasticMod > 1))
			return 7; // 3 for position, 3 for velocity, 1 for VIV phase or
			          // viscoelasticity
		else
			return 6;
	}

	/** @brief Produce the packed data to be saved
	 *
	 * The produced data can be used afterwards to restore the saved information
	 * afterwards calling Deserialize(void).
	 *
	 * Thus, this function is not processing the information that is extracted
	 * from the definition file
	 * @return The packed data
	 */
	std::vector<uint64_t> Serialize(void);

	/** @brief Unpack the data to restore the Serialized information
	 *
	 * This is the inverse of Serialize(void)
	 * @param data The packed data
	 * @return A pointer to the end of the file, for debugging purposes
	 */
	uint64_t* Deserialize(const uint64_t* data);

#ifdef USE_VTK
	/** @brief Produce a VTK object
	 * @return The new VTK object
	 */
	vtkSmartPointer<vtkPolyData> getVTK() const;

	/** @brief Save the line on a VTK (.vtp) file
	 * @param filename The output file name
	 * @throws output_file_error If VTK reports
	 * vtkErrorCode::FileNotFoundError, vtkErrorCode::CannotOpenFileError
	 * or vtkErrorCode::NoFileNameError
	 * @throws invalid_value_error If VTK reports
	 * vtkErrorCode::UnrecognizedFileTypeError or vtkErrorCode::FileFormatError
	 * @throws mem_error If VTK reports
	 * vtkErrorCode::OutOfDiskSpaceError
	 * @throws unhandled_error If VTK reports
	 * any other error
	 */
	void saveVTK(const char* filename) const;
#endif
};

} // ::moordyn
