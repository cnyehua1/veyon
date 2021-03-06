/*
 * VeyonConnection.cpp - implementation of VeyonConnection
 *
 * Copyright (c) 2008-2018 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - http://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include <rfb/rfbclient.h>

#include "VncFeatureMessageEvent.h"
#include "VeyonConnection.h"
#include "SocketDevice.h"


rfbBool handleVeyonMessage( rfbClient* client, rfbServerToClientMsg* msg )
{
	auto connection = reinterpret_cast<VeyonConnection *>( VncConnection::clientData( client, VeyonConnection::VeyonConnectionTag ) );
	if( connection )
	{
		return connection->handleServerMessage( client, msg->type );
	}

	return false;
}



static rfbClientProtocolExtension* __veyonProtocolExt = nullptr;



VeyonConnection::VeyonConnection( VncConnection* vncConnection ):
	m_vncConnection( vncConnection ),
	m_user(),
	m_userHomeDir()
{
	if( __veyonProtocolExt == nullptr )
	{
		__veyonProtocolExt = new rfbClientProtocolExtension;
		__veyonProtocolExt->encodings = nullptr;
		__veyonProtocolExt->handleEncoding = nullptr;
		__veyonProtocolExt->handleMessage = handleVeyonMessage;

		rfbClientRegisterExtension( __veyonProtocolExt );
	}

	if( m_vncConnection )
	{
		connect( m_vncConnection, &VncConnection::connectionEstablished, this, &VeyonConnection::registerConnection, Qt::DirectConnection );
	}
}



VeyonConnection::~VeyonConnection()
{
	unregisterConnection();
}



void VeyonConnection::sendFeatureMessage( const FeatureMessage& featureMessage )
{
	if( m_vncConnection.isNull() )
	{
		qCritical( "VeyonConnection::sendFeatureMessage(): cannot call enqueueEvent as m_vncConnection is invalid" );
		return;
	}

	m_vncConnection->enqueueEvent( new VncFeatureMessageEvent( featureMessage ) );
}



bool VeyonConnection::handleServerMessage( rfbClient* client, uint8_t msg )
{
	if( msg == rfbVeyonFeatureMessage )
	{
		SocketDevice socketDev( VncConnection::libvncClientDispatcher, client );
		FeatureMessage featureMessage( &socketDev );
		if( featureMessage.receive() == false )
		{
			qDebug( "VeyonConnection: could not receive feature message" );

			return false;
		}

		qDebug() << "VeyonConnection: received feature message"
				 << featureMessage.command()
				 << "with arguments" << featureMessage.arguments();

		emit featureMessageReceived( featureMessage );

		return true;
	}
	else
	{
		qCritical( "VeyonConnection::handleServerMessage(): "
				"unknown message type %d from server. Closing "
				"connection. Will re-open it later.", static_cast<int>( msg ) );
	}

	return false;
}



void VeyonConnection::registerConnection()
{
	if( m_vncConnection.isNull() == false )
	{
		m_vncConnection->setClientData( VeyonConnectionTag, this );
	}
}



void VeyonConnection::unregisterConnection()
{
	if( m_vncConnection.isNull() == false )
	{
		m_vncConnection->setClientData( VeyonConnectionTag, nullptr );
	}
}
