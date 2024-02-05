struct numbers{
	int kids;
	int secs;
};

program SLEEP_CONTROL {
    version SLEEP_CONTROL_VERSION {
        int set_sleep_duration(numbers) = 1;
    } = 1;
}=0x12345678;
