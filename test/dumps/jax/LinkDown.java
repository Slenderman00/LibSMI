/*
 * This Java file has been generated by smidump 0.2.12. Do not edit!
 * It is intended to be used within a Java AgentX sub-agent environment.
 *
 * $Id$
 */

import jax.AgentXOID;
import jax.AgentXVarBind;
import jax.AgentXNotification;
import java.util.Vector;

public class LinkDown extends AgentXNotification
{

    private final static long[] linkDown_OID = {1, 3, 6, 1, 6, 3, 1, 1, 5, 3};
    private static AgentXVarBind snmpTrapOID_VarBind =
        new AgentXVarBind(snmpTrapOID_OID,
                          AgentXVarBind.OBJECTIDENTIFIER,
                          new AgentXOID(linkDown_OID));

    private final static long[] OID1 = {1, 3, 6, 1, 2, 1, 2, 2, 1, 1};
    private final static AgentXOID ifIndex_OID = new AgentXOID(OID1);
    private final static long[] OID2 = {1, 3, 6, 1, 2, 1, 2, 2, 1, 7};
    private final static AgentXOID ifAdminStatus_OID = new AgentXOID(OID2);
    private final static long[] OID3 = {1, 3, 6, 1, 2, 1, 2, 2, 1, 8};
    private final static AgentXOID ifOperStatus_OID = new AgentXOID(OID3);


    public LinkDown(IfEntry ifEntry, IfEntry ifEntry, IfEntry ifEntry) {
        AgentXOID oid;
        AgentXVarBind varBind;

        // add the snmpTrapOID object
        varBindList.addElement(snmpTrapOID_VarBind);

        // add the ifIndex columnar object
        oid = ifIndex_OID;
        oid.appendImplied(ifEntry.getInstance());
        varBind = new AgentXVarBind(oid,
                                    AgentXVarBind.INTEGER,
                                    ifEntry.get_ifIndex());
        varBindList.addElement(varBind);

        // add the ifAdminStatus columnar object
        oid = ifAdminStatus_OID;
        oid.appendImplied(ifEntry.getInstance());
        varBind = new AgentXVarBind(oid,
                                    AgentXVarBind.INTEGER,
                                    ifEntry.get_ifAdminStatus());
        varBindList.addElement(varBind);

        // add the ifOperStatus columnar object
        oid = ifOperStatus_OID;
        oid.appendImplied(ifEntry.getInstance());
        varBind = new AgentXVarBind(oid,
                                    AgentXVarBind.INTEGER,
                                    ifEntry.get_ifOperStatus());
        varBindList.addElement(varBind);
    }

    public Vector getVarBindList() {
        return varBindList;
    }

}

