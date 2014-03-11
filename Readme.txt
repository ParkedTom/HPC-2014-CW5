Improvement Approach
==============================================================================
Keeping in mind that the pixel latency was going to be the evaluation metric the first approach was to divide up the read and write into blocks, rather than processing the whole image in one go. Alongside this, it was attempted to process multiple levels per iteration by extending the kernel reviewed. Following this, parallelism effects were explored.

OpenCL was considered at the beginning but as a later stage, however as early improvements took longer than expected it was excluded.


Testing Methodology
==============================================================================
In order to test that the program at all ran, a stream of 0s generated from /dev/null were used. If that completed, three images were used for variation. They were:
	- Lenna, 512x512 http://imgur.com/Rtj0qh3
	- Stars, 1632x1170 http://imgur.com/sVzF7JM
	- Modern art style masterpiece from paint 8192x8192 http://imgur.com/2u8vZAx
For all three cases, the number of levels and bits as they passed. Most of the time, varying the number of bits in working cases didn't show any bugs, however different levels would regularly bring up buggy behaviour, especially for large images. 



Work partitioning (planning, design, testing, coding)
===============================================================================
All planning and design was done together. After the initial planning session, we regularly synced up on progress and changed planning depending on outcome and time required (based on how much was actually needed). The coding was divided as equal as possible and was then included in the final result if and when working. 

Testing was continiously cross referenced by both as we use different operating systems and double check for bugs. When bugs incurred they were reviewed together but mainly updated by the author of the functionality.
