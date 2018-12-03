pipeline {
  agent {
    node {
      label 'master-node'
    }

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