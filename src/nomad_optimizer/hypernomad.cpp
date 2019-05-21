/* ------------------------------------------------------------------------------*/
/*  HYPERNOMAD - Hyper-parameter optimization of deep neural networks with NOMAD */
/*                                                                               */
/*                                                                               */
/*  This program is free software: you can redistribute it and/or modify it      */
/*  under the terms of the GNU Lesser General Public License as published by     */
/*  the Free Software Foundation, either version 3 of the License, or (at your   */
/*  option) any later version.                                                   */
/*                                                                               */
/*  This program is distributed in the hope that it will be useful, but WITHOUT  */
/*  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or        */
/*  FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License  */
/*  for more details.                                                            */
/*                                                                               */
/*  You should have received a copy of the GNU Lesser General Public License     */
/*  along with this program. If not, see <http://www.gnu.org/licenses/>.         */
/*                                                                               */
/*  You can find information on the NOMAD software at www.gerad.ca/nomad         */
/* ------------------------------------------------------------------------------*/



/*-------------------------------------------------------------------*/
/*            Example of a problem with categorical variables        */
/*-------------------------------------------------------------------*/
#include "nomad.hpp"
#include "hyperParameters.hpp"
#include <vector>
#include <memory>
using namespace std;
using namespace NOMAD;



#define USE_SURROGATE false
/*--------------------------------------------------*/
/*  user class to define categorical neighborhoods  */
/*--------------------------------------------------*/
class My_Extended_Poll : public Extended_Poll
{

private:

    // vector of signatures
    int _extended_poll_call;
    std::shared_ptr<HyperParameters> _hyperParameters;

public:

    // constructor:
    My_Extended_Poll ( Parameters & p , std::shared_ptr<HyperParameters> & hyperParameters ):
    Extended_Poll ( p ), _hyperParameters(std::move(hyperParameters))
    {
    }

    // destructor:
    virtual ~My_Extended_Poll ( void ) {}

    // construct the extended poll points:
    virtual void construct_extended_points ( const Eval_Point &);

};


/*------------------------------------------*/
/*            NOMAD main function           */
/*------------------------------------------*/
int main ( int argc , char ** argv )
{

    // NOMAD initializations:
    begin ( argc , argv );

    // display:
    Display out ( cout );
    out.precision ( DISPLAY_PRECISION_STD );

    std::string hyperParamFile="";
    if ( argc > 1 )
        hyperParamFile = argv[1];

    try
    {

        // parameters creation:
        Parameters p ( out );

        std::shared_ptr<HyperParameters> hyperParameters = std::make_shared<HyperParameters>(hyperParamFile);

// For testing getNeighboors
        NOMAD::Point X0 = hyperParameters->getValues( ValueType::CURRENT_VALUE);
        std::vector<HyperParameters> neighboors = hyperParameters->getNeighboors(X0);
        // for ( const auto & n : neighboors )
        //     n.display();
        
        p.set_DISPLAY_DEGREE( 3 );

        p.set_DIMENSION( static_cast<int>(hyperParameters->getDimension()) );
        p.set_X0( hyperParameters->getValues( ValueType::CURRENT_VALUE) );
        p.set_BB_INPUT_TYPE( hyperParameters->getTypes() );
        p.set_LOWER_BOUND( hyperParameters->getValues( ValueType::LOWER_BOUND ) );
        p.set_UPPER_BOUND( hyperParameters->getValues( ValueType::UPPER_BOUND ) );

        std::vector<size_t> indexFixedParams = hyperParameters->getIndexFixedParams();
        for ( auto i : indexFixedParams )
            p.set_FIXED_VARIABLE( static_cast<int>(i) );

        // Each block forms a VARIABLE GROUP in Nomad
        std::vector<std::set<int>> variableGroupsIndices = hyperParameters->getVariableGroupsIndices();
        for ( auto aGroupIndices : variableGroupsIndices )
            p.set_VARIABLE_GROUP( aGroupIndices );

        p.set_BB_OUTPUT_TYPE ( hyperParameters->getBbOutputType() );
        p.set_BB_EXE( hyperParameters->getBB() );

        p.set_MAX_BB_EVAL( static_cast<int>(hyperParameters->getMaxBbEval()) );
        p.set_LH_SEARCH(0, 5);
        p.set_EXTENDED_POLL_TRIGGER ( 10 , false );
        
        p.set_DISPLAY_STATS("bbe ( sol ) obj");
        p.set_STATS_FILE("stats.txt","bbe ( sol ) obj");
        p.set_HISTORY_FILE("history.txt");

        // parameters validation:
        p.check();

        // extended poll:
        My_Extended_Poll ep ( p , hyperParameters );

        // algorithm creation and execution:
        Mads mads ( p , NULL , &ep , NULL , NULL );
        
        std::cout << "===================================================" << std::endl;
        std::cout << "            STARTING NOMAD OPTIMIZATION            " << std::endl;
        std::cout << "===================================================" << std::endl << std::endl;
        
        mads.run();
    }
    catch ( exception & e ) {
        string error = string ( "HYPER NOMAD has been interrupted: " ) + e.what();
        if ( Slave::is_master() )
            cerr << endl << error << endl << endl;
    }


    Slave::stop_slaves ( out );
    end();

    return EXIT_SUCCESS;
}

/*--------------------------------------*/
/*  construct the extended poll points  */
/*      (categorical neighborhoods)     */
/*--------------------------------------*/
void My_Extended_Poll::construct_extended_points ( const Eval_Point & x)
{

    // Get the neighboors of the point (an update of the hyper parameters structure is performed)
    std::vector<HyperParameters> neighboors = _hyperParameters->getNeighboors(x);

    for ( auto & nHyperParameters : neighboors )
    {
        size_t nDim = nHyperParameters.getDimension();
        vector<bb_input_type> nBbit = nHyperParameters.getTypes();

        NOMAD::Point nLowerBound = nHyperParameters.getValues( ValueType::LOWER_BOUND );
        NOMAD::Point nUpperBound = nHyperParameters.getValues( ValueType::UPPER_BOUND );
        NOMAD::Point nX = nHyperParameters.getValues( ValueType::CURRENT_VALUE );

        // Create a parameter to obtain a signature for this neighboor
        NOMAD::Parameters nP ( _p.out() );
        nP.set_DIMENSION( static_cast<int>(nDim) );
        nP.set_X0 ( nX );
        nP.set_LOWER_BOUND( nLowerBound );
        nP.set_UPPER_BOUND( nUpperBound );

        nP.set_BB_INPUT_TYPE( nBbit );
        nP.set_MESH_TYPE( NOMAD::XMESH );  // Need to force set XMesh

        std::vector<size_t> indexFixed = nHyperParameters.getIndexFixedParams();
        for ( auto i : indexFixed )
            nP.set_FIXED_VARIABLE( static_cast<int>(i) );

        // Each block forms a NOMAD VARIABLE GROUP
        std::vector<std::set<int>> variableGroupsIndices = nHyperParameters.getVariableGroupsIndices();
        for ( auto aGroupIndices : variableGroupsIndices )
            nP.set_VARIABLE_GROUP( aGroupIndices );


        // Some parameters come from the original problem definition
        nP.set_BB_OUTPUT_TYPE( _p.get_bb_output_type() );
        nP.set_BB_EXE( _p.get_bb_exe() );
        // Check is need to create a valid signature
        nP.check();

        // The signature to be registered with the neighboor point
        add_extended_poll_point ( nX , *(nP.get_signature()) );
    }
}