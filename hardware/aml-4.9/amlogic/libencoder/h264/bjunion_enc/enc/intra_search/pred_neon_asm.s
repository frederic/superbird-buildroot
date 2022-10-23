    .section .text
    .global  pred_cost_i16
pred_cost_i16:
    stmdb      sp!, {r4 - r10, lr}
    @@@@@@@@@@@@@@@@@@@@@@data stack:(highend)lr,r12-r4(lowend)@@@@@@@@@@@@@@@@\n\t"
    sub        sp,   sp,  #512        @allocate int16 res[256] at stack

    mov        r4,   sp               @r4: pointer to res[0]
    mov        r5,   #8               @r5: j
    1:
    VLD4.32    {d18[0], d19[0], d20[0], d21[0]},  [r0], r1 
    VLD4.32    {d18[1], d19[1], d20[1], d21[1]},  [r0], r1
    vtrn.16    q9, q10                          
    vtrn.8     d18, d19                          
    vtrn.8     d20, d21                          
    VLD4.8     {d22, d23, d24, d25},  [r2]!   

    @m0:q9, m1:q13, m2:q11, m3:q14                
    vsubl.u8   q13, d19, d23                      @m1= org[1]-pred[1]
    vsubl.u8   q9, d18, d22                      @m0= org[0]-pred[0]
    vsubl.u8   q14, d21, d25                      @m3= org[3]-pred[3]
    vsubl.u8   q11, d20, d24                      @m2= org[2]-pred[2]

    @m0 += m3; m3 = m0 - (m3 << 1)             
    vadd.s16   q9, q9, q14                      @m0 += m3
    vshl.s16   q14, q14, #1                      @m3 = m3<<1
    vsub.s16   q14, q9, q14                      @m3 = m0 - m3

    @m1 += m2, m2 = m1 - (m2 << 1)             
    vadd.s16   q13, q13, q11                      @m1 += m2
    vshl.s16   q11, q11, #1                      @m2 = m2 << 1 
    vsub.s16   q11, q13, q11                      @m2 = m1 - m2

    vadd.s16   q10, q11, q14                      @pres[1] = m2 + m3
    vsub.s16   q12, q14, q11                      @pres[3] = m3 - m2
    vsub.s16   q11, q9, q13                      @pres[2] = m0 - m1
    vadd.s16   q9, q9, q13                      @pres[0] = m0 + m1
    @VST4.16    {q9, q10, q11, q12}, [r4]!         
    VST4.16    {d18, d20, d22, d24}, [r4]!         
    VST4.16    {d19, d21, d23, d25}, [r4]!         

    subs       r5,   r5,  #1           
    bgt        1b                      
    
    @r5:j r6:cost, r7:k 
    VMOV.U32   d30, #0                  
    mov        r5, #4                  
    mov        r7, #2                  
    mov        r8, #32                
    mov        r9, #0

    2: 
    add        r4, sp, r9              
    add        r10,sp, r9 
    vld1.16    {q9}, [r4], r8          @m0
    vld1.16    {q10}, [r4], r8          @m1
    vld1.16    {q11}, [r4], r8          @m2
    vld1.16    {q12}, [r4], r8          @m3

    @m0 += m3, m3 = m0 - (m3<<1)       
    vadd.s16   q9, q9, q12              @m0 += m3
    vshl.s16   q12, q12, #1              @m3 = m3<<1
    vsub.s16   q12, q9, q12              @m3 = m0 - m3

    @m1 += m2, m2 = m1 - (m2<<1)       
    vadd.s16   q10, q10, q11              @m1 += m2
    vshl.s16   q11, q11, #1              @m2 = m2<<1
    vsub.s16   q11, q10, q11              @m2 = m1 - m2

    @pres[0] = m0 = m0 + m1            
    vadd.s16   q9, q9, q10
    vst1.16    {q9}, [r10]!             

    @m1 = m0 - (m1 << 1);              
    @cost += ((m1 > 0) ? m1 : -m1);    
    vshl.s16   q10, q10, #1              @m1 = m1<<1
    vsub.s16   q10, q9, q10              @m1 = m0 - m1

    vabs.s16   q9, q9                  @q9 = |m0|
    vshr.u64   q9, q9, #16             @@????????????
    vaddl.u16  q9, d18, d19              @m0[3] + m0[7]
    vabs.s16   q10, q10                  @q10 = |m1|
    vaddl.u16  q10, d20, d21              @m1[0:7] + m1[8:15]

    @m3 = m2 + m3;                     
    @m2 = m3 - (m2 << 1);              
    @cost += ((m3 > 0) ? m3 : -m3);    
    @cost += ((m2 > 0) ? m2 : -m2);    

    vadd.s16   q12, q11, q12              
    vshl.s16   q11, q11, #1              @m2 = m2<<1
    vsub.s16   q11, q12, q11              

    vabs.s16   q11, q11                  
    vaddl.u16  q11, d22, d23              @m2[0:7] + m2[8:15]
    vabs.s16   q12, q12                  
    vaddl.u16  q12, d24, d25              @m3[0:7] + m3[8:15]

    @cost, m1+m2+m3
    @cost, m0[3] + m0[7] + m0[11] + m0[15]
    vadd.u32   q10, q10, q9              
    vadd.u32   q10, q10, q11              
    vadd.u32   q10, q10, q12              
    @vpaddl.u32 q10, q10                 
    vadd.u32   d20, d20, d21                 
    vpaddl.u32 d20, d20                 
    vadd.u32   d30,d30,d20
    vmov.u32   r6, d30[0]               
    subs       r6, r3, r6, lsr #1
    blo        7f                      @end

    @k cycle
    add        r9, r9, #16             
    subs       r7, r7, #1              
    bgt        2b                      

    @j cycle update                    
    add        r9, r9, #96          @96=128-32
    mov        r7, #2                  
    subs       r5, r5, #1              
    bgt        2b                      

    mov        r4,   sp                
    mov        r5,   #4                
    4:                                 
    ldrsh      r6,   [r4]
    ldrsh      r7,   [r4,  #8]        @r6: m0; r7: m1
    ldrsh      r8,   [r4,  #24]
    ldrsh      r9,   [r4,  #16]       @r8: m3; r9: m2
    
    asr        r6,   r6,  #2           
    add        r6,   r6,  r8,  asr #2 @r6:  m0
    sub        r8,   r6,  r8,  asr #1 @r8:  m3
    
    asr        r7,   r7,  #2           
    add        r7,   r7,  r9,  asr #2 @r7:  m1
    sub        r9,   r7,  r9,  asr #1 @r9:  m2
    
    add        r10,  r6,  r7           
    strh       r10,  [r4]              
    sub        r10,  r6,  r7           
    strh       r10,  [r4, #16]         
    add        r10,  r8,  r9           
    strh       r10,  [r4, #8]          
    sub        r10,  r8,  r9           
    strh       r10,  [r4, #24]         

    add        r4,   r4,  #128         
    subs       r5,   r5,  #1           
    bgt        4b                      
    
              
    mov        r4,   sp                
    mov        r5,   #4
	mov        r7,   #128                
    5:                                 
    vld1.16    {q9}, [r4], r7
    vld1.16    {q10},[r4], r7
    vld1.16    {q11},[r4], r7
    vld1.16    {q12},[r4], r7

    @m0 += m3,m3 = m0 - (m3 << 1)      
    vadd.s16   q9, q9, q12             @m0 += m3
    vshl.s16   q12,q12,#1              @m3 = m3<<1
    vsub.s16   q12,q9, q12             @m3 = m0 - m3

    @m1 += m2, m2 = m1 - (m2<<1)       
    vadd.s16   q10,q10,q11             @m1 += m2
    vshl.s16   q11,q11,#1              @m2 = m2<<1
    vsub.s16   q11,q10,q11             @m2 = m1 - m2
    
    @m0 = m0 + m1, cost += ((m0 >= 0) ? m0 : -m0)
    @m1 = m0 - (m1 << 1),cost += ((m1 >= 0) ? m1 : -m1)
    vadd.s16   q9, q9, q10             @m0 += m1
    vshl.s16   q10,q10,#1              @m1 = m1<<1
    vsub.s16   q10,q9, q10             @m1 = m0 - m1
    vabs.s16   q9, q9                  
    vabs.s16   q10,q10                  

    @m3 = m2 + m3, cost += ((m3 >= 0) ? m3 : -m3)
    @m2 = m3 - (m2 << 1),cost += ((m2 >= 0) ? m2 : -m2)
    vadd.s16   q12,q11,q12              @m3 += m2
    vshl.s16   q11,q11, #1              @m2 = m2<<1
    vsub.s16   q11,q12, q11             @m2 = m3 - m2
    vabs.s16   q11,q11                  
    vabs.s16   q12,q12                  
    
	vaddl.u16  q9, d18,d19             @m0[0] + m0[8]
    vaddl.u16  q10,d20,d21             @m1[0] + m1[8]
    vaddl.u16  q11,d22,d23             @m2[0] + m2[8]
    vaddl.u16  q12,d24,d25             @m3[0] + m3[8]

    vaddl.u32  q9, d18,d20             @m0+ m1
    vaddl.u32  q11,d22,d24             @m2+ m3
    vaddl.u32  q9, d18,d22             @m0+m1+m2+m3
    VADD.U32   d30,d18,d30             

    VMOV.U32   r6,   d30[0]            
    lsr        r6,   r6,  #1           
    cmp        r6,   r3
    bgt        7f                                            
    6:                                 
    add        r4,   sp,  #16           
    subs       r5,   r5,  #1           
    bgt        5b                      

    7:                                 
    VMOV.U32   r6,   d30[0]            
    lsr        r6,   r6,  #1           
    mov        r0,   r6            
    add        sp,   sp,  #512         

    ldmia      sp!, {r4 - r10, pc}
    @ENDP  @ |pred_cost_i16|


    .section .text
    .global  pred_cost_i4
pred_cost_i4:
    stmdb      sp!, {r4 - r5, lr}
    VLD4.8     {d4[0], d5[0], d6[0], d7[0]},  [r0], r1 
    VLD4.8     {d4[1], d5[1], d6[1], d7[1]},  [r0], r1
    VLD4.8     {d4[2], d5[2], d6[2], d7[2]},  [r0], r1
    VLD4.8     {d4[3], d5[3], d6[3], d7[3]},  [r0], r1
    VLD4.8     {d0, d1, d2, d3},  [r2]

    @m0 = org[0] - pred[0]                 
    @m3 = org[3] - pred[3]                 
    vsubl.u8   q4, d4, d0                  
    vsubl.u8   q5, d5, d1                  
    vsubl.u8   q2, d6, d2                  
    vsubl.u8   q3, d7, d3                  

    @m0 += m3, m3 = m0 - (m3 << 1)          
    @m1 += m2, m2 = m1 - (m2 << 1)          
    vadd.s16   d8, d8, d6                  
    vshl.s16   d6, d6, #1                  
    vsub.s16   d6, d8, d6                  

    vadd.s16   d10,d10,d4                  
    vshl.s16   d4, d4, #1                  
    vsub.s16   d4, d10,d4                  
    
    @pres[0] = m0 + m1                     
    @pres[2] = m0 - m1                     
    @vadd.s16   q0, q4, q5                  
    @vsub.s16   q1, q4, q5                  
    vadd.s16   d0, d8, d10                 
    vsub.s16   d2, d8, d10                 

    @pres[1] = m2 + m3                     
    @pres[3] = m3 - m2                     
    @vadd.s16   q4, q2, q3                  
    @vsub.s16   q5, q3, q2                  
    vadd.s16   d1, d4, d6                  
    vsub.s16   d3, d6, d4                  

    vtrn.16  d0, d1                        
    vtrn.16  d2, d3                        
    vtrn.32  q0, q1                        

    @m0 = pres[0],m3 = pres[12]            
    @m0 += m3, m3 = m0 - (m3 << 1)         
    vadd.s16 d0, d0, d3                    
    vshl.s16 d3, d3, #1                    
    vsub.s16 d3, d0, d3                    

    @m1 = pres[4], m2 = pres[8]            
    @m1 += m2, m2 = m1 - (m2 << 1)         
    vadd.s16 d1, d1, d2                    
    vshl.s16 d2, d2, #1                    
    vsub.s16 d2, d1, d2                    

    @pres[0] = m0 + m1, pres[8] = m0 - m1  
    @pres[4] = m2 + m3, pres[12] = m3 - m2 
    vadd.s16   d4, d0, d1                  
    vsub.s16   d6, d0, d1                  
    vadd.s16   d5, d2, d3                  
    vsub.s16   d7, d3, d2                  

    @tmp1 = *pres++, satd += ((tmp1 >= 0) ? tmp1 : -tmp1)
    vabs.s16   q2, q2                      
    vabs.s16   q3, q3                      
    vaddl.u16  q0, d4, d5                  
    vaddl.u16  q1, d6, d7                  
    vadd.u32   q0, q0, q1                  
    vadd.u32   d0, d0, d1                  
    vpaddl.u32 d0, d0                      

    vmov.u32   r5, d0[0]                   
    add        r5,  r5,  #1                 
    lsr        r5,  r5,  #1                 
    ldrh       r4,  [r3]               
    add        r4,  r4,  r5                 
    strh       r4,  [r3]              
    1:@end                                 

    ldmia      sp!, {r4 - r5, pc}
    @ENDP  @ |pred_cost_i4|
    .section .text
    .global  cost_i4_both
cost_i4_both:
    stmdb      sp!, {r4 - r8, lr}
    sub        sp, sp, #64
    mov        r4, #0
    vdup.8     q1, r4
    vdup.8     q2, r4
    vdup.8     q3, r4

    VLD4.8     {d4[0], d5[0], d6[0], d7[0]},  [r0], r1 
    VLD4.8     {d4[1], d5[1], d6[1], d7[1]},  [r0], r1
    VLD4.8     {d4[2], d5[2], d6[2], d7[2]},  [r0], r1
    VLD4.8     {d4[3], d5[3], d6[3], d7[3]},  [r0], r1

    vshl.u64   q1, q2, #32
    vadd.u32   q2, q1, q2
    vshl.u64   q1, q3, #32
    vadd.u32   q3, q1, q3

    VLD4.8     {d0, d1, d2, d3},  [r2]

    @m0 = org[0] - pred[0]                 
    @m3 = org[3] - pred[3]                 

    @vsubl.u8   q2, d6, d2                  
    @vsubl.u8   q3, d7, d3                  
    @vsubl.u8   q4, d4, d0     @q4:m0, q5:m1, q2:m2, q3:m3             
    @vsubl.u8   q5, d5, d1                  
    vsubl.u8   q4, d6, d2                  
    vsubl.u8   q5, d7, d3                  
    vsubl.u8   q3, d4, d0     @q4:m2, q5:m3, q2:m0, q3:m1             
    vsubl.u8   q2, d5, d1                  

    @m0 += m3, m3 = m0 - (m3 << 1)          
    @m1 += m2, m2 = m1 - (m2 << 1)          
    @m0:q2, m3:q5
    @m1:q3, m2:q4
    vadd.s16   q2, q2, q5                  
    vshl.s16   q5, q5, #1                  
    vsub.s16   q5, q2, q5            
    vadd.s16   q3, q3, q4                 
    vshl.s16   q4, q4, #1                  
    vsub.s16   q4, q3, q4                 
    
    @pres[0] = m0 + m1                     
    @pres[2] = m0 - m1                     
    vadd.s16   q0, q2, q3                 
    vsub.s16   q2, q2, q3                 

    @pres[1] = m2 + m3                     
    @pres[3] = m3 - m2                     
    vadd.s16   q1, q4, q5                  
    vsub.s16   q3, q5, q4                  

    vtrn.16  q0, q1                        
    vtrn.16  q2, q3                        
    vtrn.32  q0, q2                        
    vtrn.32  q1, q3                        

    @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    @mov        r4, sp                      
    @vst4.16    {d0,d2,d4,d6}, [r4]!
    @vst4.16    {d1,d3,d5,d7}, [r4]!
    @mov    r0, sp
    @mov    r1, #4
    @mov    r2, #8
    @bl     print_i16
    @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

    @m0 = pres[0],m3 = pres[12]            
    @m0 += m3, m3 = m0 - (m3 << 1)         
    vadd.s16 q0, q0, q3                    
    vshl.s16 q3, q3, #1                    
    vsub.s16 q3, q0, q3                    

    @m1 = pres[4], m2 = pres[8]            
    @m1 += m2, m2 = m1 - (m2 << 1)         
    vadd.s16 q1, q1, q2                    
    vshl.s16 q2, q2, #1                    
    vsub.s16 q2, q1, q2                    

    @pres[0] = m0 + m1, pres[8] = m0 - m1  
    @pres[4] = m2 + m3, pres[12] = m3 - m2 
    @vadd.s16   d4, d0, d1                  
    @vsub.s16   d6, d0, d1                  
    @vadd.s16   d5, d2, d3                  
    @vsub.s16   d7, d3, d2                  
    vadd.s16   q4, q0, q1                  
    vsub.s16   q1, q0, q1                  
    vadd.s16   q5, q2, q3                  
    vsub.s16   q3, q3, q2                  

    @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    @mov        r4, sp                      
    @vst1.16    {d8}, [r4]!              
    @vst1.16    {d2}, [r4]!              
    @vst1.16    {d10},[r4]!              
    @vst1.16    {d6}, [r4]!              
    @vst1.16    {d9}, [r4]!              
    @vst1.16    {d3}, [r4]!              
    @vst1.16    {d11},[r4]!              
    @vst1.16    {d7}, [r4]!              

    @mov    r0, sp
    @mov    r1, #4
    @mov    r2, #8
    @bl     print_i16
    @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

    @tmp1 = *pres++, satd += ((tmp1 >= 0) ? tmp1 : -tmp1)
    vabs.s16   q4, q4                      
    vabs.s16   q5, q5                      
    vabs.s16   q1, q1                      
    vabs.s16   q3, q3                      

    @vaddl.u16  q0, d8, d10                  
    @vaddl.u16  q4, d9, d11                  
    @vaddl.u16  q5, d2, d6                  
    @vaddl.u16  q3, d3, d7                  
    vadd.u16   q0, q4, q5
    vadd.u16   q1, q1, q3
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    @vadd.u32   q0, q0, q5                  
    @vadd.u32   d0, d0, d1                  
    @vpaddl.u32 d0, d0                      

    @vadd.u32   q3, q3, q4                  
    @vadd.u32   d6, d6, d7                  
    @vpaddl.u32 d6, d6                      

    vadd.u16    q0, q0, q1
    vpaddl.u16  q0, q0
    vpaddl.u32  q0, q0
    @vpaddl.u16  d0, d0
    @vpaddl.u16  d1, d1
    @vpaddl.u32  d0, d0
    @vpaddl.u32  d1, d1

    vmov.u32   r5, d0[0]                   
    add        r5,  r5,  #1                 
    lsr        r5,  r5,  #1                 

    ldrh       r4,  [r3]               
    add        r4,  r4,  r5                 
    strh       r4,  [r3]              
    add        r3,  r3, #2

    vmov.u32   r5, d1[0]                   
    add        r5,  r5,  #1                 
    lsr        r5,  r5,  #1                 

    ldrh       r4,  [r3]               
    add        r4,  r4,  r5                 
    strh       r4,  [r3]              
    @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    @mov       r8, sp                      
    @vst1.16   {d1}, [r8]!
    @strh      r6, [r8]!
    @strh      r7, [r8]!
    @mov    r0, sp
    @mov    r1, #6
    @mov    r2, #1
    @bl     print_i16
    @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

    1:@end                                 
    add        sp, sp, #64

    ldmia      sp!, {r4 - r8, pc}
    @ENDP  @ |cost_i4_both|
