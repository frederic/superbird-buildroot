#!/bin/sh

:<<!
project list:
1. mesong12a_skt_32_release
2. mesong12a_skt_release
3. mesong12a_u200_32_release
4. mesong12a_u200_release
5. mesonaxg_s400_32_release
6. mesonaxg_s400_32_debug
7. mesonaxg_s400_debug
8. mesonaxg_s400_release
9. mesonaxg_s400_32_emmc_release
10. mesonaxg_s400_emmc_release
11. mesonaxg_s420_32_debug
12. mesonaxg_s420_32_release
13. mesonaxg_s420_debug
14. mesonaxg_s420_release
15. mesonaxg_a113x_skt_32_release
16. mesonaxg_a113x_skt_release
17. mesonaxg_a113d_skt_32_release
18. mesonaxg_a113d_skt_release
19. mesongxl_p400_32_kernel49_release
20. mesongxl_p400_kernel49_release
21. mesongxl_p401_32_kernel49_release
22. mesongxl_p401_kernel49_release
23. mesongxl_p212_32_kernel49_release
24. mesongxl_p212_kernel49_release
25. mesongxl_p230_32_kernel49_release
26. mesongxl_p230_kernel49_release
27. mesongxl_p231_32_kernel49_release
28. mesongxl_p231_kernel49_release
29. mesongxl_p241_32_kernel49_release
30. mesongxl_p241_kernel49_release
31. mesongxm_q200_32_kernel49_release
32. mesongxm_q200_kernel49_release
33. meson8b_m200_kernel49_release
34. mesongxl_p400_release
35. mesongxl_p400_32_release
36. mesongxl_p401_release
37. mesongxl_p401_32_release
38. mesongxb_p200_release
39. mesongxb_p200_32_release
40. mesongxb_p201_release
41. mesongxb_p201_32_release
42. mesongxl_p212_release
43. mesongxl_p212_32_release
44. mesongxl_p230_release
45. mesongxl_p230_32_release
46. mesongxl_p231_release
47. mesongxl_p231_32_release
48. mesongxl_p241_release
49. mesongxl_p241_32_release
50. mesongxm_q200_release
51. mesongxm_q200_32_release
52. meson8_k200_release
53. meson8_k200b_release
54. meson8b_m200_release
55. meson8b_m201_release
56. meson8m2_n200_release
57. meson8b_m400_release
!


# this value from the project index.
# According to the project setting,  num 8 was the default value
defProject=8
curPath=${PWD}


argCnt=$#
compileMod=$1

function setupProject(){
    echo "setup the project."
    source buildroot/build/setenv.sh ${defProject}
}

function compileByArg(){
    argMod=$1
    echo "compileByArg: will compile the ${argMod}"
    if [ "$argMod" =  "" ];then
    make 
    elif [ $argMod = "uboot" ];then
    make uboot-rebuild
    elif [ $argMod = "kernel" ];then   
    make linux-rebuild 
    elif [ $argMod = "openssh" ] ; then 
    make openssh-rebuild 
    elif [ $argMod = "show" ] ; then 
    make show-targets
    else 
    make 
    fi
}

function showHowUse(){
    echo "##########++++++++++++##############################################################"
    echo "#              show how to use the runConpise Script:                              #"
    echo "#              1. source runCompile.sh // bulid all                                #"
    echo "#              2. source runCompile.sh uboot/kernel/openssh // bulid single module #"
    echo "#              Note: the default config is [8. mesonaxg_s420_32_release], you can  #"
    echo "#                    change the defProject value According to the project setting  #"
    echo "####################################################################################"
}

showHowUse
setupProject
compileByArg ${compileMod}
