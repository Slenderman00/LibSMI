/*
 * This Java file has been generated by smidump 0.4.2-pre1. Do not edit!
 * It is intended to be used within a Java AgentX sub-agent environment.
 *
 * $Id$
 */

/**
    This class represents a Java AgentX (JAX) implementation of
    the table row hlHostControlEntry defined in RMON2-MIB.

    @version 1
    @author  smidump 0.4.2-pre1
    @see     AgentXTable, AgentXEntry
 */

import jax.AgentXOID;
import jax.AgentXSetPhase;
import jax.AgentXResponsePDU;
import jax.AgentXEntry;

public class HlHostControlEntry extends AgentXEntry
{

    protected int hlHostControlIndex = 0;
    protected AgentXOID hlHostControlDataSource = new AgentXOID();
    protected AgentXOID undo_hlHostControlDataSource = new AgentXOID();
    protected long hlHostControlNlDroppedFrames = 0;
    protected long hlHostControlNlInserts = 0;
    protected long hlHostControlNlDeletes = 0;
    protected int hlHostControlNlMaxDesiredEntries = 0;
    protected int undo_hlHostControlNlMaxDesiredEntries = 0;
    protected long hlHostControlAlDroppedFrames = 0;
    protected long hlHostControlAlInserts = 0;
    protected long hlHostControlAlDeletes = 0;
    protected int hlHostControlAlMaxDesiredEntries = 0;
    protected int undo_hlHostControlAlMaxDesiredEntries = 0;
    protected byte[] hlHostControlOwner = new byte[0];
    protected byte[] undo_hlHostControlOwner = new byte[0];
    protected int hlHostControlStatus = 0;
    protected int undo_hlHostControlStatus = 0;

    public HlHostControlEntry(int hlHostControlIndex)
    {
        this.hlHostControlIndex = hlHostControlIndex;

        instance.append(hlHostControlIndex);
    }

    public int get_hlHostControlIndex()
    {
        return hlHostControlIndex;
    }

    public AgentXOID get_hlHostControlDataSource()
    {
        return hlHostControlDataSource;
    }

    public int set_hlHostControlDataSource(AgentXSetPhase phase, AgentXOID value)
    {
        switch (phase.getPhase()) {
        case AgentXSetPhase.TEST_SET:
            break;
        case AgentXSetPhase.COMMIT:
            undo_hlHostControlDataSource = hlHostControlDataSource;
            hlHostControlDataSource = value;
            break;
        case AgentXSetPhase.UNDO:
            hlHostControlDataSource = undo_hlHostControlDataSource;
            break;
        case AgentXSetPhase.CLEANUP:
            break;
        default:
            return AgentXResponsePDU.PROCESSING_ERROR;
        }
        return AgentXResponsePDU.NO_ERROR;
    }
    public long get_hlHostControlNlDroppedFrames()
    {
        return hlHostControlNlDroppedFrames;
    }

    public long get_hlHostControlNlInserts()
    {
        return hlHostControlNlInserts;
    }

    public long get_hlHostControlNlDeletes()
    {
        return hlHostControlNlDeletes;
    }

    public int get_hlHostControlNlMaxDesiredEntries()
    {
        return hlHostControlNlMaxDesiredEntries;
    }

    public int set_hlHostControlNlMaxDesiredEntries(AgentXSetPhase phase, int value)
    {
        switch (phase.getPhase()) {
        case AgentXSetPhase.TEST_SET:
            break;
        case AgentXSetPhase.COMMIT:
            undo_hlHostControlNlMaxDesiredEntries = hlHostControlNlMaxDesiredEntries;
            hlHostControlNlMaxDesiredEntries = value;
            break;
        case AgentXSetPhase.UNDO:
            hlHostControlNlMaxDesiredEntries = undo_hlHostControlNlMaxDesiredEntries;
            break;
        case AgentXSetPhase.CLEANUP:
            break;
        default:
            return AgentXResponsePDU.PROCESSING_ERROR;
        }
        return AgentXResponsePDU.NO_ERROR;
    }
    public long get_hlHostControlAlDroppedFrames()
    {
        return hlHostControlAlDroppedFrames;
    }

    public long get_hlHostControlAlInserts()
    {
        return hlHostControlAlInserts;
    }

    public long get_hlHostControlAlDeletes()
    {
        return hlHostControlAlDeletes;
    }

    public int get_hlHostControlAlMaxDesiredEntries()
    {
        return hlHostControlAlMaxDesiredEntries;
    }

    public int set_hlHostControlAlMaxDesiredEntries(AgentXSetPhase phase, int value)
    {
        switch (phase.getPhase()) {
        case AgentXSetPhase.TEST_SET:
            break;
        case AgentXSetPhase.COMMIT:
            undo_hlHostControlAlMaxDesiredEntries = hlHostControlAlMaxDesiredEntries;
            hlHostControlAlMaxDesiredEntries = value;
            break;
        case AgentXSetPhase.UNDO:
            hlHostControlAlMaxDesiredEntries = undo_hlHostControlAlMaxDesiredEntries;
            break;
        case AgentXSetPhase.CLEANUP:
            break;
        default:
            return AgentXResponsePDU.PROCESSING_ERROR;
        }
        return AgentXResponsePDU.NO_ERROR;
    }
    public byte[] get_hlHostControlOwner()
    {
        return hlHostControlOwner;
    }

    public int set_hlHostControlOwner(AgentXSetPhase phase, byte[] value)
    {
        switch (phase.getPhase()) {
        case AgentXSetPhase.TEST_SET:
            break;
        case AgentXSetPhase.COMMIT:
            undo_hlHostControlOwner = hlHostControlOwner;
            hlHostControlOwner = new byte[value.length];
            for(int i = 0; i < value.length; i++)
                hlHostControlOwner[i] = value[i];
            break;
        case AgentXSetPhase.UNDO:
            hlHostControlOwner = undo_hlHostControlOwner;
            break;
        case AgentXSetPhase.CLEANUP:
            undo_hlHostControlOwner = null;
            break;
        default:
            return AgentXResponsePDU.PROCESSING_ERROR;
        }
        return AgentXResponsePDU.NO_ERROR;
    }
    public int get_hlHostControlStatus()
    {
        return hlHostControlStatus;
    }

    public int set_hlHostControlStatus(AgentXSetPhase phase, int value)
    {
        switch (phase.getPhase()) {
        case AgentXSetPhase.TEST_SET:
            break;
        case AgentXSetPhase.COMMIT:
            undo_hlHostControlStatus = hlHostControlStatus;
            hlHostControlStatus = value;
            break;
        case AgentXSetPhase.UNDO:
            hlHostControlStatus = undo_hlHostControlStatus;
            break;
        case AgentXSetPhase.CLEANUP:
            break;
        default:
            return AgentXResponsePDU.PROCESSING_ERROR;
        }
        return AgentXResponsePDU.NO_ERROR;
    }
}

