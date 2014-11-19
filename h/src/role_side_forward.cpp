// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3, or (at your option)
 any later version.

 This code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this code; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "role_side_forward.h"
#include "sample_player.h"

#include "bhv_chain_action.h"
#include "bhv_basic_offensive_kick.h"
#include "bhv_basic_move.h"
#include <rcsc/action/body_kick_one_step.h>
#include <rcsc/action/body_smart_kick.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/debug_client.h>
#include <vector> 

#include <rcsc/common/logger.h>

using namespace rcsc;

const std::string RoleSideForward::NAME( "SideForward" );

/*-------------------------------------------------------------------*/
/*!

 */
namespace {
rcss::RegHolder role = SoccerRole::creators().autoReg( &RoleSideForward::create,
                                                       RoleSideForward::NAME );
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
RoleSideForward::execute( PlayerAgent * agent )
{
    bool kickable = agent->world().self().isKickable();
    if ( agent->world().existKickableTeammate()
         && agent->world().teammatesFromBall().front()->distFromBall()
         < agent->world().ball().distFromSelf() )
    {
        kickable = false;
    }

    if(agent->world().self().pos().x>40 && agent->world().self().pos().y<5 && agent->world().self().pos().y>-5){
        kickable = false;
        doKick(agent);
    }
    else{
        kickable = true;
    }

    if ( kickable )
    {
        if(!doPass(agent))
            doMove( agent );
    }
    else
    {
            doMove( agent );
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
RoleSideForward::doKick( PlayerAgent * agent )
{
    if ( Bhv_ChainAction().execute( agent ) )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (execute) do chain action" );
        agent->debugClient().addMessage( "ChainAction" );
        return;
    }

    Bhv_BasicOffensiveKick().execute( agent );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
RoleSideForward::doMove( PlayerAgent * agent )
{
    Bhv_BasicMove().execute( agent );
}


bool
RoleSideForward::PassToPoint( PlayerAgent * agent, int receiver )
{
    double first_speed = ServerParam::i().ballSpeedMax()/ 1.1;
    if(receiver<0||receiver>12){
        return false;
    }

    Vector2D target_point = Vector2D(agent->world().ourPlayer(receiver)->pos().x, agent->world().ourPlayer(receiver)->pos().y);

    agent->debugClient().addMessage( "pass" );
    agent->debugClient().setTarget( target_point );

    int kick_step = ( agent->world().gameMode().type() != GameMode::PlayOn
                      && agent->world().gameMode().type() != GameMode::GoalKick_
                      ? 1
                      : 1 );
    if ( ! Body_SmartKick( target_point,
                           first_speed,
                           first_speed * 3,
                           kick_step ).execute( agent ) )
    {
        if ( agent->world().gameMode().type() != GameMode::PlayOn
             && agent->world().gameMode().type() != GameMode::GoalKick_ )
        {
            first_speed = std::min( agent->world().self().kickRate() * ServerParam::i().maxPower(),
                                    first_speed );
            Body_KickOneStep( target_point,
                              first_speed * 3
                              ).execute( agent );
        }
        else
        {
            return false;
        }
    }

/*    if ( agent->config().useCommunication()
         && receiver != Unum_Unknown )
    {
        Vector2D target_buf = target_point - agent->world().self().pos();
        target_buf.setLength( 1.0 );

        agent->addSayMessage( new PassMessage( receiver,
                                               target_point + target_buf,
                                               agent->effector().queuedNextBallPos(),
                                               agent->effector().queuedNextBallVel() ) );
    }
*/
    return true;
}

bool
RoleSideForward::doPass( PlayerAgent * agent )
{
    return SamplePlayer().PassToBestPlayer(agent);
}

int 
RoleSideForward::ClosestPlayerToBall(PlayerAgent * agent){
    double mindis = 20;
    
    int array[10] = {0}, barray[10] = {-999};
    //int mindisunum;
    for(int i=2; i<=11; i++){
        if(agent->world().ourPlayer(i)!=NULL){
            if(agent->world().ourPlayer(i)->distFromBall() < mindis && IsOccupiedForPassing(agent, i)){
                //mindis = agent->world().ourPlayer(i)->distFromBall();
                //mindisunum = i;

                array[i]=i;
                barray[i]=agent->world().ourPlayer(i)->distFromBall();
            }
        }
    }
    
    int tempp, tempp1;
    for(int i=0; array[i+1]!=0; i++){

        if(barray[i]>barray[i+1])
        {
            tempp=barray[i+1];
            barray[i+1]=barray[i];
            barray[i]=tempp;

            tempp1=array[i+1];
            array[i+1]=array[i];
            array[i]=tempp1;
        }

    }
    if(array[1]!=0)
        return array[1];
    else
        return array[0];
}

bool
RoleSideForward::IsOccupiedForPassing(PlayerAgent * agent, int ourPl){
    //Body_TurnToPoint( target ).execute( agent );
        if(ourPl>0||ourPl<12){
        return false;
    }
Vector2D target = Vector2D(agent->world().ourPlayer(ourPl)->pos().x, agent->world().ourPlayer(ourPl)->pos().y);
    if(abs(target.x)>55 || abs(target.y)>15)
        return 0;

    Neck_TurnToPoint( target ).execute(agent );
    const WorldModel & wm = agent->world();
    
    if ( target.x > wm.offsideLineX() + 1.0 ){
        // offside players are rejected.
        return false;
    }

/*    for(int i=1;i<=11;i++){
        if(wm.theirPlayer(i)!=NULL){
        Vector2D player_pos = wm.ourPlayer(i)->pos();
        }
    }


    if(wm.ourPlayer(i)!=NULL){
        Vector2D player_pos = wm.ourPlayer(i)->pos();
        if( AreSamePoints(player_pos, target, buffer) && i!=wm.self().unum() )
            return i;
    }
    return 0;
    */
    return true;
}
