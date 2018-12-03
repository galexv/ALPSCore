/** Declarative pipeline for Jenkins on Pauli */

pipeline {
  agent {
    node {
      label 'master-node'
    }

  }

    // Unfortunately, this has to be set manually.
    parameters {
        string(name: 'COMPILER', defaultValue: 'gcc_5.4.0', description: 'Compiler to use')
        string(name: 'MPI_VERSION', defaultValue: 'MPI_OFF', description: 'MPI Version to link with')
    }

    environment {
        COMPILER = "$COMPILER"
        MPI_VERSION = "$MPI_VERSION"
    }
    
    stages {
    stage('Configure') {
      steps {
        sh '''export PHASE=cmake
./common/build/build.pauli.jenkins.sh
'''
      }
    }
    stage('Build') {
      steps {
        sh '''export PHASE=make
./common/build/build.pauli.jenkins.sh
'''
      }
    }
    stage('Test') {
      steps {
        sh '''export PHASE=test
./common/build/build.pauli.jenkins.sh
'''
      }
    }
  }
}