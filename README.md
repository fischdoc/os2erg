# os2erg

## run

1. **Server side**:
     ```bash
     ./test_server
     ```

2. **Client side**:
     ```bash
     ./test_client <host> <num_of_children> <seconds>
     ```
3. **Graceful Shutdown**:
    - Press ctrl+c at any point (server side) to shut down the children first, and the parent last.
