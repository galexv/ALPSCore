pipeline {
  agent {
    node {
      label 'master-node'
    }

  }
  stages {
    stage('Config gcc') {
      parallel {
        stage('Config gcc') {
          environment {
            COMPILER = 'gcc_5.4.0'
            MPI_VERSION = 'MPI_OFF'
          }
          steps {
            ws(dir: 'ws')
            sh '''pwd
export PHASE=cmake
./common/build/build.pauli.jenkins.sh
'''
          }
        }
        stage('Config clang') {
          environment {
            COMPILER = 'clang_3.4.2'
            MPI_VERSION = 'MPI_OFF'
          }
          steps {
            ws(dir: 'ws')
            sh '''pwd
export PHASE=cmake
./common/build/build.pauli.jenkins.sh
'''
          }
        }
      }
    }
  }
}