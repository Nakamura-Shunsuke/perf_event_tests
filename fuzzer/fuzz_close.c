#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/mman.h>

#include <signal.h>

#include "../include/perf_event.h"

#include "perf_fuzzer.h"
#include "fuzzer_determinism.h"
#include "fuzzer_logging.h"
#include "fuzzer_stats.h"

#include "fuzz_close.h"

void close_event(int i, int from_sigio) {

	int result;
	int unmap_before_close;

	/* Exit if no events */
	if (i<0) return;

	unmap_before_close=rand()%2;

	if (ignore_but_dont_skip.close) return;

	/* here to avoid race in overflow where we munmap or close */
	/* but event still marked active */

	event_data[i].active=0;

	/* unmap any associated memory */
	/* we tried not doing this randomly, */
	/* but rapidly ran out of memory */
	if ((event_data[i].mmap) && unmap_before_close) {
		munmap(event_data[i].mmap,event_data[i].mmap_size);
		if ((!from_sigio) && (logging&TYPE_MMAP)) {
			sprintf(log_buffer,"U %d %d %p\n",
				event_data[i].fd,
				event_data[i].mmap_size,
				event_data[i].mmap);
			write(log_fd,log_buffer,strlen(log_buffer));
		}
		event_data[i].mmap=0;
	}

	stats.close_attempts++;
	result=close(event_data[i].fd);
	if (result==0) {
		stats.close_successful++;
		stats.current_open--;
	}

	if ((!from_sigio) && (logging&TYPE_CLOSE)) {
		sprintf(log_buffer,"C %d\n",event_data[i].fd);
		write(log_fd,log_buffer,strlen(log_buffer));
	}

	/* sometimes unmap after we've closed */
	if ((event_data[i].mmap) && !unmap_before_close) {
		munmap(event_data[i].mmap,event_data[i].mmap_size);
		if ((!from_sigio) && (logging&TYPE_MMAP)) {
			sprintf(log_buffer,"U %d %d %p\n",
				event_data[i].fd,
				event_data[i].mmap_size,
				event_data[i].mmap);
			write(log_fd,log_buffer,strlen(log_buffer));
		}
		event_data[i].mmap=0;
	}

	active_events--;


}

void close_random_event(void) {

	int i;

	i=find_random_active_event();

	close_event(i,0);

}
