#!/bin/bash

bin_folder=$1

if [[ ! -d "${bin_folder}" ]]; then
    echo "binary folder \'${bin_folder}\' does not exist"
    exit -1    
fi

source_base_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source_folder_file="${source_base_dir}/source_folders.txt"

if [[ ! -r "${source_folder_file}" ]]; then
    echo "no source folder file \'${source_folder_file}\' exists"
    exit -1
fi

trident_platform="simulation"
log_level=2
app_log="ON"
layer2_log="ON"
layer1_log="OFF"
layer0_log="OFF"
build_type="debug"
trident_simulation_ipc_dummy_file_folder="${bin_folder}/ipc_comm"

pushd "${bin_folder}" > /dev/null 2>&1

overwrite_all="false"
while read -r src_dir ; do
    src_dir_full="${source_base_dir}/${src_dir}"
    if [[ ! -d "${src_dir_full}" ]]; then
        echo "Source directory \"${src_dir}\" does not exist"
        continue
    fi
    bin_dir=$(echo ${src_dir} | sed 's/\//_/g')
    bin_dir_full="$(pwd)/${bin_dir}"
    
    if [[ -d "${bin_dir_full}" ]]; then
        if [[ "true" == ${overwrite_all} ]]; then
            rm -rf "${bin_dir_full}"
        else
            do_overwrite=""
            
            jmp_next_folder="false"
            while true; do
                echo -e "Binary directory \"${bin_dir}\" already exists, overwrite? [y/n/a] \c" 
                read do_overwrite < /dev/tty
            
                case $do_overwrite in
                    [nN]* ) jmp_next_folder="true"; break;;
                    [yY]* ) rm -rf "${bin_dir_full}"; break;;
                    [aA]* ) rm -rf "${bin_dir_full}"; overwrite_all="true"; break;; 
                esac
            done
            if [[ "true" == "${jmp_next_folder}" ]]; then
                continue;
            fi
        fi 
    fi

    mkdir ${bin_dir_full}

    pushd "${bin_dir}" > /dev/null 2>&1
        echo -e "Building \"${src_dir}\" ... \c"
        cmake -G "Unix Makefiles" -DTRIDENT_PLATFORM="${trident_platform}" -DTRIDENT_SIMULATION_IPC_DUMMY_FILE_FOLDER="${trident_simulation_ipc_dummy_file_folder}" -DTRIDENT_LOGGING_LVL="${log_level}" -DTRIDENT_LOGGING_APPLICATION_ENABLED="${app_Log}" -DTRIDENT_LOGGING_LAYER2_ENABLED="${layer2_log}" -DTRIDENT_LOGGING_LAYER1_ENABLED="${layer1_log}" -DTRIDENT_LOGGING_LAYER0_ENABLED="${layer0_log}" -DCMAKE_BUILD_TYPE="${build_type}" "${src_dir_full}" > cmake.log 2>&1
        if [[ 0 -ne $? ]]; then
            echo "cmake failed"
        else
            make -j > build.log  2>&1
            if [[ 0 -ne $? ]]; then
                echo -e "make failed"
            else
                echo -e "success"
            fi
        fi        
    popd > /dev/null 2>&1
done < "$source_folder_file"

popd > /dev/null 2>&1
