/*
 * This Java file has been generated by smidump 0.2.8. Do not edit!
 * It is intended to be used within a Java AgentX sub-agent environment.
 *
 * $Id$
 */

/**
    This class represents a Java AgentX (JAX) implementation of
    the table tokenRingMLStats2Table defined in RMON2-MIB.

    @version 1
    @author  smidump 0.2.8
    @see     AgentXTable
 */

import java.util.Vector;

import jax.AgentXOID;
import jax.AgentXVarBind;
import jax.AgentXResponsePDU;
import jax.AgentXSetPhase;
import jax.AgentXTable;
import jax.AgentXEntry;

public class TokenRingMLStats2Table extends AgentXTable
{

    // entry OID
    private final static long[] OID = {1, 3, 6, 1, 2, 1, 16, 1, 5, 1};

    // constructor
    public TokenRingMLStats2Table()
    {
        oid = new AgentXOID(OID);

        // register implemented columns
        columns.addElement(new Long(1));
        columns.addElement(new Long(2));
    }

    public AgentXVarBind getVarBind(AgentXEntry entry, long column)
    {
        AgentXOID oid = new AgentXOID(getOID(), column, entry.getInstance());

        switch ((int)column) {
        case 1: // tokenRingMLStatsDroppedFrames
        {
            long value = ((TokenRingMLStats2Entry)entry).get_tokenRingMLStatsDroppedFrames();
            return new AgentXVarBind(oid, AgentXVarBind.COUNTER32, value);
        }
        case 2: // tokenRingMLStatsCreateTime
        {
            long value = ((TokenRingMLStats2Entry)entry).get_tokenRingMLStatsCreateTime();
            return new AgentXVarBind(oid, AgentXVarBind.GAUGE32, value);
        }
        }

        return null;
    }

    public int setEntry(AgentXSetPhase phase,
                        AgentXEntry entry,
                        long column,
                        AgentXVarBind vb)
    {

        switch ((int)column) {
        }

        return AgentXResponsePDU.NOT_WRITABLE;
    }

}

